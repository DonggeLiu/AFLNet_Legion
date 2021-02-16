#include "mcts.h"

/* Hyper-parameters */
gdouble RHO = 0.0025;  //or sqrt(2)
u32 MIN_SAMPLES = 1;
u32 MAX_SAMPLES = 100;
u32 CONEX_TIMEOUT = 0;
gboolean PERSISTENT = FALSE;
gboolean COVERAGE_ONLY = TRUE;
enum score_function SCORE_FUNCTION = UCT;


/* Statistics */
uint ROUND = 0;


/* ============================================== TreeNode Functions ============================================== */

TreeNodeData* new_tree_node_data (u32 response_code, enum node_colour colour, u32* path, u32 path_len)
{
    TreeNodeData* tree_node_data;
    tree_node_data = g_new(TreeNodeData, 1); //  allocates 1 element of TreeNode

    // set property
    tree_node_data->id = response_code;
    /* NOTE: The path of each node is the prefix of the path in which the node is found */
    tree_node_data->path = malloc(sizeof(u32)*path_len);
    memcpy(tree_node_data->path, path, sizeof(u32)*path_len);
    tree_node_data->path_len = path_len;
    //NOTE: This is probably not needed, left it here in case it comes in handy later.
    tree_node_data->colour = colour;

    if (colour == Golden) {assert(tree_node_data->id == 999);}
    else  {assert(tree_node_data->id == tree_node_data->path[tree_node_data->path_len - 1]);}

    // set statistics
    tree_node_data->selected = 0;
    tree_node_data->discovered = 0;
    // tree_node_data->stats.selected_seed_index = 0;
    tree_node_data->seeds = NULL;
    tree_node_data->region_indices = NULL;
    tree_node_data->seeds_count = 0;

    //TODO: Detect terminations (i.e. leaves) with some response code, and mark them fully explored.
    // e.g. predefine a set of termination code
    //TOASK: Given the last node of a past communication, would it have any child if we give it more inputs?
    tree_node_data->fully_explored = FALSE;
    //NOTE: No way to know if we have found all possible inputs to fuzz a node.
    tree_node_data->exhausted = FALSE;

    return tree_node_data;
}


TreeNode* new_tree_node(TreeNodeData* tree_data)
{
    GNode* tree_node;
    tree_node = g_node_new (tree_data);
    return tree_node;
}


TreeNodeData* get_tree_node_data(TreeNode* tree_node)
{
    return (TreeNodeData*) tree_node->data;
}

TreeNode* get_simulation_child(TreeNode* tree_node)
{
  TreeNodeData* node_data = get_tree_node_data(tree_node);
  TreeNode* sim_child = node_data->simulation_child;

  assert(node_data->colour != Golden && node_data->colour != Black);
  assert(get_tree_node_data(sim_child)->colour == Golden);

  return sim_child;
}

double tree_node_exploitation_score(TreeNode* tree_node)
{
    TreeNodeData* node_data = get_tree_node_data(tree_node);

    if (!node_data->selected) return INFINITY;

    g_assert(node_data->selected);

    return (double) node_data->discovered / node_data->selected;
}

double seed_exploitation_score(TreeNode* tree_node, int seed_index)
{
    seed_info_t* target_seed = get_tree_node_data(tree_node)->seeds[seed_index];

    if (!target_seed->selected) return INFINITY;

    return (double) target_seed->discovered / target_seed->selected;
}

double tree_node_exploration_score(TreeNode* tree_node)
{
    if (G_NODE_IS_ROOT(tree_node)) return INFINITY;
    g_assert(tree_node->parent);
    TreeNodeData* node_data = get_tree_node_data(tree_node);
    if (!node_data->selected) { return INFINITY; }
    TreeNodeData* parent_data = get_tree_node_data(tree_node->parent);
    log_debug("%lf * sqrt(2 * log(%lf) / %lf", RHO, (double) parent_data->selected, (double) node_data->selected);

    return  RHO * sqrt(2 * log((double) parent_data->selected) / (double) node_data->selected);
}

double seed_exploration_score(TreeNode* tree_node, int seed_index)
{
    seed_info_t* target_seed = get_tree_node_data(tree_node)->seeds[seed_index];
    TreeNodeData* node_data = get_tree_node_data(tree_node);
    if (!target_seed->selected) { return INFINITY; }
    return RHO * sqrt(2 * log((double)node_data->selected)/target_seed->selected);
}

double tree_node_score(TreeNode* tree_node)
{
    /*
     * If using the random score function, return a random number
     * Else compute the score of a node:
     *  1. If the node is fully explored, return -INFINITY
     *  2. If the node is root, return INFINITY
     *  3. If the node fits the fish bone optimisation, return -INFINITY
     *  4. If the node has not been selected before, return INFINITY
     *  5. If the node has been selected before, apply the UCT function
     */
    if (SCORE_FUNCTION == Random) return g_rand_int(RANDOM_NUMBER_GENERATOR);

    if (get_tree_node_data(tree_node)->fully_explored) return -INFINITY;

//    if (G_NODE_IS_ROOT(tree_node))  return INFINITY;

    // if (fits_fish_bone_optimisation(tree_node)) return -INFINITY;

//    if (!get_tree_node_data(tree_node)->selected)  return INFINITY;

    double exploit_score = tree_node_exploitation_score(tree_node);
    double explore_score = tree_node_exploration_score(tree_node);

    return exploit_score + explore_score;
}

double seed_score(TreeNode* tree_node, int seed_index)
{
    // return g_rand_int(RANDOM_NUMBER_GENERATOR);

    if (SCORE_FUNCTION == Random) return g_rand_int(RANDOM_NUMBER_GENERATOR);

    seed_info_t* target_seed = get_tree_node_data(tree_node)->seeds[seed_index];

//    if (!target_seed->selected)  return INFINITY;

    double exploit_score = seed_exploitation_score(tree_node, seed_index);
    double explore_score = seed_exploration_score(tree_node, seed_index);

    return exploit_score + explore_score;
}

gboolean is_leaf(TreeNode* tree_node) {
    /*
     * If the node is a phantom, then it is not a leaf
     * If the node has no child, it is a leaf
     * If the node has only a simulation child, it is a leaf
     * Else it is not
     */
    return (get_tree_node_data(tree_node)->colour != Purple)
           && (G_NODE_IS_LEAF(tree_node)
               || ((g_node_n_children(tree_node) == 1) &&
                   get_tree_node_data(g_node_first_child(tree_node))->colour == Golden));
}

//gboolean fits_fish_bone_optimisation(TreeNode * tree_node)
//{
//    /* NOTE: We might not be able to know for sure if a node/subtree is fully explored.
//     *  One optimisation is to predefine a set of termination response codes.
//     * Fish bone optimisation: if a simulation child
//     * has only one sibling X who is not fully explored,
//     * and X is not white (so that all siblings are found)
//     * then do not simulate from that simulation child but only from X
//     * as all new paths can only come from X
//     */
////    gboolean is_simul = (get_tree_node_data(tree_node)->colour == Golden);
//
//    return FALSE;
//}

TreeNode* best_child(TreeNode* tree_node)
{
    gdouble max_score = -INFINITY;
    u32 number_of_children = g_node_n_children(tree_node);
    u32 number_of_ties = 0;
    u32 ties[number_of_children];

    if (number_of_children == 1) {
//        g_printf("Only one child: ");
//        tree_node_print(g_node_nth_child(tree_node, 0));
        return g_node_nth_child(tree_node, 0);
    }

    for (u32 child_index = 0; child_index < number_of_children; child_index++) {
        gdouble score = tree_node_score(g_node_nth_child(tree_node, child_index));
//        g_printf("max score: %lf, current score: %lf\n", max_score, score);
        if (score < max_score) continue;
        if (score > max_score) number_of_ties = 0;
        max_score = score;
        ties[number_of_ties] = child_index;
        number_of_ties ++;
//        g_printf("max score: %lf, current score: %lf, number of ties: %d, current child index: %d\n",
//               max_score, score, number_of_ties, child_index);
    }

    if (number_of_ties == 1)
    {
//        g_printf("Only one candidate: ");
//        tree_node_print(g_node_nth_child(tree_node, ties[0]));
        return g_node_nth_child(tree_node, ties[0]);
    }

//    g_printf("Best child has %d candidates: \n", number_of_ties);
//    for (int i = 0; i < number_of_ties; ++i) {
//        tree_node_print(g_node_nth_child(tree_node, ties[i]));
//    }
//    g_printf("\n");
    return g_node_nth_child(tree_node, ties[g_rand_int_range(RANDOM_NUMBER_GENERATOR, 0, number_of_ties)]);
}

seed_info_t* best_seed(TreeNode* tree_node)
{
    gdouble max_score = -INFINITY;
    u32 number_of_seeds = get_tree_node_data(tree_node)->seeds_count;
    u32 number_of_ties = 0;
    u32 ties[number_of_seeds];

    if (number_of_seeds == 1) {
        g_printf("Only one seed: ");
        return get_tree_node_data(tree_node)->seeds[0];
    }

    for (u32 seed_index = 0; seed_index < number_of_seeds; seed_index++) {
        int score = seed_score(tree_node, seed_index);

        if (score < max_score) continue;
        if (score > max_score) number_of_ties = 0;
        max_score = score;
        ties[number_of_ties] = seed_index;
        number_of_ties ++;
    }

    if (number_of_ties == 1)
    {
        g_printf("Only one candidate: ");
        return get_tree_node_data(tree_node)->seeds[ties[0]];
    }

    u32 winner_index = ties[g_rand_int_range(RANDOM_NUMBER_GENERATOR, 0, number_of_ties)];
    return get_tree_node_data(tree_node)->seeds[winner_index];
}

TreeNode* exists_child(TreeNode* tree_node, u32 target_response_code)
{
    TreeNode* child_node = g_node_first_child(tree_node);

    while (child_node)
    {
        // tree_node_print(child_node);
        if (get_tree_node_data(child_node)->id == target_response_code)  return child_node;
        child_node = g_node_next_sibling(child_node);
    }
    return NULL;
}

TreeNode* append_child(TreeNode* tree_node, u32 child_response_code, enum node_colour colour, u32* path, u32 path_len)
{
    TreeNode* child = g_node_append_data(tree_node, new_tree_node_data(child_response_code, colour, path, path_len));
    if (colour == White) get_tree_node_data(child)->simulation_child = append_child(child, 999, Golden, path, path_len);

    return child;
}

void print_reversed_path(TreeNode* tree_node)
{
    do {
        g_printf("%u ", get_tree_node_data(tree_node)->id);
        tree_node = tree_node->parent;
    } while (tree_node);
    g_printf("\n");
}

void print_path(TreeNode* tree_node)
{
  TreeNodeData* tree_node_data = get_tree_node_data(tree_node);
  for (u32 i = 0; i < tree_node_data->path_len; i++) {
      g_printf("%u ", tree_node_data->path[i]);
  }
  g_print("\n");
}

char* node_path_str(TreeNode* tree_node)
{
  TreeNodeData* tree_node_data = get_tree_node_data(tree_node);

    char* a_str = NULL;
    int a_str_len = asprintf(&a_str, "%u", tree_node_data->path[0]);

    for (int i = 1; i < tree_node_data->path_len; ++i) {
      assert(a_str_len != -1);
      a_str_len = asprintf(&a_str, "%s, %u", a_str, tree_node_data->path[i]);
    }
  return a_str;
}

int colour_encoder(enum node_colour colour) {
    int colour_code = 0;
    switch (colour) {
        case White:
            colour_code = 37;
            break;
        case Red:
            colour_code = 31;
            break;
        case Golden:
            colour_code = 33;
            break;
        case Purple:
            colour_code = 35;
            break;
        case Black:
            colour_code = 90;
            break;
    }
    assert(colour_code);
    return colour_code;
}

void tree_log(TreeNode* tree_node, TreeNode* mark_node, int indent, int found)
{
//    log_Message* message = (log_Message*) message_init();
  char* message = NULL;
  for (int i = 0; i < indent-1; ++i) message_append(&message, "|  ");
  if (indent) message_append(&message, "|-- ");
  if (indent>MAX_TREE_LOG_DEPTH) {
    message_append(&message, " ... ");
    log_info(message);
    return;
  }
  message_append(&message, tree_node_repr(tree_node));
  if (tree_node == mark_node && found >= 0) message_append(&message, "\033[1;32m <=< found\033[0m");
  log_info(message);

  if (g_node_n_children(tree_node)) indent++;
  for (int i = 0; i < g_node_n_children(tree_node); ++i) {
    tree_log(g_node_nth_child(tree_node, i), mark_node, indent, found);
  }
}

char* tree_node_repr(TreeNode* tree_node)
{
  TreeNodeData* tree_node_data = get_tree_node_data(tree_node);
  char* message = NULL;

  message_append(&message, "\033[1;%dm %03u: %lf [%lf + %lf] {%03u, %03u}\033[0m",
           colour_encoder(tree_node_data->colour),
           tree_node_data->id,
           tree_node_score(tree_node),
           tree_node_exploitation_score(tree_node),
           tree_node_exploration_score(tree_node),
           tree_node_data->selected,
           tree_node_data->discovered);
  return message;
}


seed_info_t* construct_seed_with_queue_entry(void *q)
{
  seed_info_t* seed = (seed_info_t*) ck_alloc (sizeof(seed_info_t));
  seed->q = q;
  seed->selected = 0;
  seed->discovered = 0;
  return seed;
}

void add_seed_to_node(seed_info_t* seed, u32 matching_region_index, TreeNode * node)
{
  assert(get_tree_node_data(node)->colour == Golden);
  /*NOTE: Figure out M2 at here so that we don't have to do it repeatedly when the same queue_entry is selected*/
  struct queue_entry *q = seed->q;
  region_t* regions = q->regions;
  TreeNodeData* tree_node_data = get_tree_node_data(node);

  // TOASK: Should we allocate more spaces to avoid repeated reallocation?
  TreeNodeData* node_data = get_tree_node_data(node);
  node_data->seeds = (void **) ck_realloc (node_data->seeds, (node_data->seeds_count + 1) * sizeof(void *));
  node_data->seeds[node_data->seeds_count] = (seed_info_t *) seed;
  node_data->region_indices = (u32*) ck_realloc (node_data->region_indices, (node_data->seeds_count + 1) * sizeof(u32));
  node_data->region_indices[node_data->seeds_count] = matching_region_index;
  seed->parent_index = node_data->seeds_count;
  node_data->seeds_count++;

  seed_info_t* node_seed = node_data->seeds[node_data->seeds_count-1];
  struct queue_entry* node_q = node_seed->q;
  region_t node_region = node_q->regions[node_data->region_indices[seed->parent_index]];

  log_debug("[ADD_SEED_TO_NODE] Region ID of node    : %u", node_data->region_indices[node_seed->parent_index]);
  log_debug("[ADD_SEED_TO_NODE] Region count of queue: %u", node_q->region_count);
  assert(node_data->region_indices[node_seed->parent_index] <= node_q->region_count);

  log_debug("[ADD_SEED_TO_NODE] Region state count: %u", node_region.state_count);
  log_debug("[ADD_SEED_TO_NODE] Node path length  : %u", tree_node_data->path_len);

  log_debug("[ADD_SEED_TO_NODE] Region states: %s", u32_array_to_str(node_region.state_sequence, node_region.state_count));
  log_debug("[ADD_SEED_TO_NODE] Node path    : %s", u32_array_to_str(tree_node_data->path, tree_node_data->path_len));
  assert(node_region.state_count >= tree_node_data->path_len);
  assert(!memcmp(node_region.state_sequence, tree_node_data->path, tree_node_data->path_len));
}

u32* collect_region_path(region_t region, u32* path_len)
{
    *path_len = region.state_count;
    return region.state_sequence;
}

/* ============================================== TreeNode Functions ============================================== */
/* ================================================ MCTS Functions ================================================ */

TreeNode* select_tree_node(TreeNode* parent_tree_node)
{
    get_tree_node_data(parent_tree_node)->selected++;
    while (get_tree_node_data(parent_tree_node)->colour != Golden) {
//        tree_node_print(parent_tree_node);
//        g_print("\n");
        parent_tree_node = best_child(parent_tree_node);
        /* NOTE: Selected stats propagation of nodes along the selection path is done here */
        get_tree_node_data(parent_tree_node)->selected++;
        assert(parent_tree_node);
    }
    return parent_tree_node;
}


seed_info_t* select_seed(TreeNode* tree_node_selected)
{
    seed_info_t* seed = best_seed(tree_node_selected);
    /* NOTE: Selected stats propagation of the seed is done here */
    seed->selected++;
    return seed;
}


TreeNode* Initialisation()
{
    u32 path[] = {0};
    TreeNode* root = new_tree_node(new_tree_node_data(0, White, path, 1));
    get_tree_node_data(root)->simulation_child = append_child(root, 999, Golden, path, 1);
    char log_file[100];
    snprintf(log_file, sizeof(log_file), "%s", getenv("AFLNET_LEGION_LOG"));
    log_info("LOG PATH: %s", log_file);
    log_add_fp(fopen(log_file, "w+"),2);
    log_set_quiet(TRUE);
    return root;
}

seed_info_t* Selection(TreeNode** tree_node)
{
    ROUND += 1;
    log_info("[SELECTION] ==========< ROUND %03d >==========", ROUND);
    assert(G_NODE_IS_ROOT(*tree_node));

    *tree_node = select_tree_node(*tree_node);
    log_info("[SELECTION] Selected node   : %s", tree_node_repr(*tree_node));
    log_info("[SELECTION] Selected parent : %s", tree_node_repr((*tree_node)->parent));
//    prepare_path_str(*tree_node);
//    g_printf("\tTree node selected: ");
//    tree_node_print(tree_node);
//    struct queue_entry * seed_selected = NULL;
    seed_info_t* seed_selected = select_seed(*tree_node);
    struct queue_entry* q = seed_selected->q;

    log_info("[SELECTION] Selection seed  : %s", q->fname);
    for (int i = 0; i < q->region_count; ++i) {
      log_debug("[SELECTION] Seed region %2d  : %s",
               i, u32_array_to_str(q->regions[i].state_sequence, q->regions[i].state_count));
    }
    TreeNodeData* tree_node_data = get_tree_node_data(*tree_node);
    log_info("[SELECTION] Selection path  : %s", node_path_str(*tree_node));
    log_debug("[SELECTION] Selection region: %s",
             u32_array_to_str(q->regions[tree_node_data->region_indices[seed_selected->parent_index]].state_sequence,
                              q->regions[tree_node_data->region_indices[seed_selected->parent_index]].state_count));
    return seed_selected;
}

char* Simulation(TreeNode* target)
{
    assert(get_tree_node_data(target)->colour == Golden);
    return NULL;
}

TreeNode* Expansion(TreeNode* tree_node, struct queue_entry* q, u32* response_codes, u32 len_codes, gboolean* is_new)
{
  TreeNode* parent_node;
  *is_new = FALSE;
  u32 matching_region_index = 0;
  gboolean matched_last_code = FALSE;

  // Construct seed with queue_entry q
  seed_info_t* seed = construct_seed_with_queue_entry(q);

  // Check if the response code sequence is new
  // And add the new queue entry to each node along the paths
  assert(response_codes[0] == 0);
  for (u32 path_index = 1; path_index < len_codes; path_index++) {
    parent_node = tree_node;
    log_debug("[MCTS-EXPANSION] === Matching code %03u at index %u ===", response_codes[path_index], path_index);
    if ((path_index != len_codes - 1) && get_tree_node_data(tree_node)->fully_explored){
      //NOTE: If the node is not the last in an execution sequence, but somehow it was marked as fully explored
      //  then correct this mistake
      get_tree_node_data(tree_node)->fully_explored = FALSE;
      log_debug("[MCTS-EXPANSION] code %03u at index %u is not fully explored anymore",
               response_codes[path_index], path_index);
    }

    if (!(tree_node = exists_child(tree_node, response_codes[path_index]))){
      log_debug("[MCTS-EXPANSION] Detected a new path at code %03u at index %u ",
               response_codes[path_index], path_index);
      *is_new = TRUE;
    }

    for (u32 region_index = matching_region_index + matched_last_code;
         (region_index < q->region_count) && (region_index < len_codes);
         ++region_index)
    {
      region_t region = q->regions[region_index];
      //NOTE: some regions are NULL for some reason, but we should not expect a matching tree node anyway
      // Because NULL regions only show up after the region with the longest state sequence,
      // which means their region indices will not be greater than len_codes
//        assert(region);
      // NOTE: The state_count of a path preserving region should always
      //  be greater than or equal to the path_len (path_index+1) of its matching node
      //  Node Colour:
      //    White if the last state of the region's state sequence is the
      log_debug("[MCTS-EXPANSION] Matching code %03u at index %3u: %s",
               response_codes[path_index], path_index, u32_array_to_str(response_codes, path_index+1));
      log_debug("[MCTS-EXPANSION] With region at index %3u      : %s (Queue Entry %s)",
                region_index, u32_array_to_str(region.state_sequence, region.state_count), q->fname);
      if (path_index+1 <= region.state_count) {
//        exact_match = (path_index+1 == region.state_count);
        matched_last_code = response_codes[path_index] == region.state_sequence[region.state_count-1];
        matching_region_index = region_index;
        if (matched_last_code) {log_debug("[MCTS-EXPANSION] Exact Match found");}
        else  {log_debug("[MCTS-EXPANSION] Partial Match found");}
        break;
      }
    }

    if (*is_new)  {
      enum node_colour colour;
      if (matched_last_code)  {colour = White;}
      else  {colour = Black;}
      tree_node = append_child(parent_node, response_codes[path_index], colour, response_codes, path_index+1);
      log_debug("[MCTS-EXPANSION] Add a new child %s of parent %s",
               tree_node_repr(tree_node), tree_node_repr(parent_node));
    }
    if (matched_last_code) {
      TreeNodeData* tree_node_data = get_tree_node_data(tree_node);
      if (tree_node_data->colour == Black && path_index + 1 != len_codes) {
        // NOTE: Flip a node if this is black and is not an termination point
        //  But still was considered as Black
        log_debug("[MCTS-EXPANSION] Flipping node from Black to White: %s", tree_node_repr(tree_node));
//        tree_log(ROOT, tree_node, 0, 0);
        tree_node_data->colour = White;
        tree_node_data->simulation_child = append_child(tree_node, 999, Golden, response_codes, path_index+1);
        log_debug(tree_node_repr(tree_node));

//        tree_log(ROOT, tree_node, 0, 0);
      }
      if (tree_node_data->colour == White && q->regions[matching_region_index].state_count < len_codes) {
        //NOTE: Only add seed to node if node colour is White  (Otherwise there is no simulation child)
        // and the matching region is not the last region in the q, (otherwise M2 count is 0)
        add_seed_to_node(seed, matching_region_index, get_simulation_child(tree_node));
        TreeNodeData* sim_data = get_tree_node_data(get_simulation_child(tree_node));
        seed_info_t* seed = sim_data->seeds[sim_data->seeds_count-1];
        struct queue_entry* new_q = (struct queue_entry*) seed->q;
        region_t region = q->regions[matching_region_index];
        log_debug("The following two paths should match");
        log_debug("[MCTS-EXPANSION] Region %d: %s",
                 matching_region_index, u32_array_to_str(region.state_sequence, region.state_count));
        log_debug("[MCTS-EXPANSION] Node %d: %s",
                 tree_node_data->id, u32_array_to_str(tree_node_data->path, tree_node_data->path_len));
        assert(region.state_count >= tree_node_data->path_len);
        assert(!memcmp(region.state_sequence, tree_node_data->path,tree_node_data->path_len));
        log_debug("Seed appended at index %u of the simulation child of node %s , with region id %u",
                 sim_data->seeds_count-1, tree_node_repr(tree_node),
                 sim_data->region_indices[seed->parent_index]);
      }
    }
    TreeNodeData* tree_node_data = get_tree_node_data(tree_node);
    log_debug("[MCTS-EXPANSION] Node %s path:%s",
             tree_node_repr(tree_node), u32_array_to_str(tree_node_data->path, tree_node_data->path_len));

      /* NOTE: Assert the path of each node is saved correctly */
      assert(tree_node_data->path_len == path_index+1);
      assert(!memcmp(tree_node_data->path, response_codes, sizeof(u32)*tree_node_data->path_len));
  }
  parent_node = tree_node;
  log_debug("[MCTS-EXPANSION] Node: %s", tree_node_repr(tree_node));
  log_debug("[MCTS-EXPANSION] Node's path:%s", u32_array_to_str(get_tree_node_data(tree_node)->path,
                                                               get_tree_node_data(tree_node)->path_len));
  log_debug("[MCTS-EXPANSION] Exec's path:%s", u32_array_to_str(response_codes, len_codes));
  get_tree_node_data(tree_node)->fully_explored = is_leaf(tree_node);
  log_debug("[MCTS-EXPANSION] Node: %s", tree_node_repr(tree_node));
  if (G_NODE_IS_ROOT(tree_node->parent)) {
    log_debug("");
  }

  /*NOTE: Stats propagation along the execution path is done here*/
  while (parent_node) {
      get_tree_node_data(parent_node)->discovered += *is_new;
      parent_node = parent_node->parent;
  }

  /* NOTE: Stats propagation of the seed is done here */
  seed->discovered += *is_new;
//  seed->selected += 1;
  return tree_node;
}

void Propagation(TreeNode* leaf_selected, seed_info_t* seed_selected, gboolean is_new)
{
  log_info("[PROPAGATION] Back-propagating from SimNode: %s", tree_node_repr(leaf_selected));
  log_info("[PROPAGATION] The parent of the SimNode is : %s", tree_node_repr(leaf_selected->parent));

  TreeNode* leaf_parent = leaf_selected;
  TreeNodeData* tree_node_data = get_tree_node_data(leaf_parent);
  tree_node_data->discovered += is_new;


//  while (leaf_parent) {
//    TreeNodeData* tree_node_data = get_tree_node_data(leaf_parent);
//    tree_node_data->selected += 1;
//    log_info("[PROPAGATION] Back-propagated: %s", tree_node_repr(leaf_parent));
//    leaf_parent = leaf_parent->parent;
//  }
//  seed_selected->discovered += is_new;
//  seed_selected->selected += 1;

  tree_log(ROOT, leaf_selected, 0, is_new);
}

void parent(TreeNode* child, TreeNode** parent){
    /*
     * Takes a pointer to a child node, and a pointer to the pointer of a NULL
     * Then assigned the pointer to the parent of the child to the second parameter
     */
    parent = &child->parent;
}

/* ================================================ MCTS Functions ================================================ */

//gboolean is_fully_explored(TreeNode *  tree_node)
//{
//    /*
//     * Check if a node is fully explored:
//     *  1. If not in PERSISTENT mode, return the exact stats of the tree node
//     *  2. If in PERSISTENT mode, but the root is not fully explored, return the exact stats
//     *  3. If in PERSISTENT mode and the root is fully explored, but the node is root, return the exact stats
//     *  4. Else a node is fully explored if it is a non-Golden leaf, or it is exhausted
//     */
//    TreeNodeData * tree_node_data = get_tree_node_data(tree_node);
//    if (!PERSISTENT)    return tree_node_data->fully_explored;
//
//    if (!get_tree_node_data(g_node_get_root(tree_node))->fully_explored)  return tree_node_data->fully_explored;
//
//    if (G_NODE_IS_ROOT(tree_node))  return tree_node_data->fully_explored;
//
//    return (is_leaf(tree_node) && tree_node_data->colour != Golden) || tree_node_data->exhausted;
//}

//void prepare_path_str(TreeNode* tree_node)
//{
//  u32 path_len = 0;
//  u32* path = collect_node_path(tree_node, &path_len);
//
//  u32 path_str_len = 0;
//  for (u32 i = 0; i < path_len; i++) {
//    path_str_len += (int)((ceil(log10(path[i]))+1)*sizeof(char));
//  }
//
//  log_info("Node path len: %d", path_len);
//  for (int i = 0; i < path_len; ++i) {
//
//    log_info("Node path (%d/%d): %d", i, path_len, path[i]);
//    g_printf("%d, ", path[i]);
//    printf("%d, ", path[i]);
//  }
//
//  u32 n = 0, m = 0;
//
//  char path_str[100] = {0};
//  for (u32 i = 0; i < path_len; i++) {
//    m = snprintf(&path_str[n], 100-n, "%d", path[i]);
//    if (m<0) {
//      exit(1);
//    }
//    n += m;
//  }
//
////  g_printf("%s", path_str);
//  log_info("Node path: %s", path_str);
//}

//u32* collect_node_path(TreeNode* tree_node, u32* path_len)
//{
//  u32* path = NULL;
//  u32* reversed_path = NULL;
//
//  *path_len = 0;
//  u32 path_size = 0;
//
//  //NOTE: If the tree_node is Golden, then its path is the same as its parent
//  if (get_tree_node_data(tree_node)->colour == Golden) {
//    assert(tree_node->parent);
//    tree_node = tree_node->parent;
//    assert(get_tree_node_data(tree_node)->colour != Golden);
//  }
//
//  // NOTE: The condition is tree_node->parent instead of tree_node,
//  //  because we don't want to include the id of ROOT in the path,
//  //  given the id of ROOT is always 0 and does not represent any valid response code.
//  while (tree_node->parent) {
//    //NOTE: dynamically expanding the size of array
//    // g_printf("%d of %d: %d\n", *path_len, path_size, (*path_len) >= (path_size));
//    if ((*path_len) >= (path_size)) {
//      path_size += 100;
////      g_printf("%d\n", path_size);
//      u32* new_reversed_path = ck_alloc(path_size * sizeof(u32));
//      if (new_reversed_path) {
//        for (u32 i = 0; i < *path_len; ++i) {
//          new_reversed_path[i] = reversed_path[i];
//        }
//        // TOASK: free causes error
////                ck_free(reversed_path);
////                int * reversed_path = new_reversed_path;
//        reversed_path = new_reversed_path;
//      }
//      else {
//        g_print("Insufficient Memory when collecting path");
//        exit(0);
//      }
//    }
//
//    // collect addresses from leaf to root
//    reversed_path[*path_len] = get_tree_node_data(tree_node)->id;
//    *path_len += 1;
//    tree_node = tree_node->parent;
//  }
////    g_printf("%d of %d\n", *path_len, path_size);
//
////    for (u32 i = 0; i < *path_len; i++) {
////        g_printf("%d ", reversed_path[i]);
////    }
//
////    return reversed_path;
//  // TOASK: free causes error
//  // ck_free(reversed_path);
//
//  path = ck_alloc((*path_len) * sizeof(u32));
//  for (u32 i = 0; i < *path_len; i++) {
//    path[i] = reversed_path[(*path_len) - i - 1];
//  }
//  return path;
//}
/* EOF */
