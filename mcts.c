#define _GNU_SOURCE
#include "mcts.h"


/* Hyper-parameters */
gdouble RHO = 1.414;  //or sqrt(2)
u32 MAX_TREE_LOG_DEPTH = 5;
/* Not in use */
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

    int absent_in_khmn_nodes = 0;
    khint_t k = kh_put_hmn(khmn_nodes, response_code, &absent_in_khmn_nodes);

    if (absent_in_khmn_nodes) {
      log_info("RES_CODE %u does not exist yet", response_code);
      kh_value(khmn_nodes, k) = 0;
    } else {
      log_info("RES_CODE %u exists", response_code);
    }

    if (colour == Golden) {
      log_assert(tree_node_data->id == 999,
                 "[NEW_TREE_NODE_DATA] The ID of SimNode is not 999: %03u\n", tree_node_data->id);
    } else {
      log_assert(tree_node_data->id == tree_node_data->path[tree_node_data->path_len - 1],
                 "[NEW_TREE_NODE_DATA] The ID of RspNode does not match the last code in its path: %03u != %03u",
                 tree_node_data->id, tree_node_data->path[tree_node_data->path_len - 1]);
    }

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
  char* message_tree_node_repr_tree_node = tree_node_repr(tree_node);
  char* message_tree_node_repr_sim_node = tree_node_repr(tree_node);
  log_assert(sim_child != NULL, "[GET_SIMULATION_CHILD] No sim child of %s", message_tree_node_repr_tree_node);
  log_assert(node_data->colour != Golden && node_data->colour != Black,
             "[GET_SIMULATION_CHILD] %s should not have a child SimNode %s",
             message_tree_node_repr_tree_node, message_tree_node_repr_sim_node);
  log_assert(get_tree_node_data(sim_child)->colour == Golden,
             "[GET_SIMULATION_CHILD] Golden is not the colour of the sim child (%s) of %s",
             message_tree_node_repr_sim_node, message_tree_node_repr_tree_node);
  free(message_tree_node_repr_tree_node);
  free(message_tree_node_repr_sim_node);
  message_tree_node_repr_tree_node = NULL;
  message_tree_node_repr_sim_node = NULL;
  return sim_child;
}

double tree_node_exploitation_score(TreeNode* tree_node)
{
    TreeNodeData* node_data = get_tree_node_data(tree_node);

    if (!node_data->selected) return INFINITY;

    log_assert(node_data->selected, "[TREE_NODE_EXPLOITATION_SCORE] Node not selected, causing div by 0 error");

    return (double) node_data->discovered / node_data->selected;
}

double seed_exploitation_score(TreeNode* tree_node, int seed_index)
{
    seed_info_t* target_seed = get_tree_node_data(tree_node)->seeds[seed_index];

    if (!target_seed->selected) return INFINITY;

    log_assert(target_seed->selected, "[SEED_EXPLOITATION_SCORE] Seed not selected, causing div by 0 error");

    return (double) target_seed->discovered / target_seed->selected;
}

double tree_node_exploration_score(TreeNode* tree_node)
{
    if (G_NODE_IS_ROOT(tree_node)) return INFINITY;
    log_assert(tree_node->parent != NULL,
               "[TREE_NODE_EXPLORATION_SCORE] The parent of node (%03u) does not exist",
               get_tree_node_data(tree_node)->id);
    TreeNodeData* node_data = get_tree_node_data(tree_node);
    if (!node_data->selected) { return INFINITY; }
    TreeNodeData* parent_data = get_tree_node_data(tree_node->parent);
//    log_trace("[TREE_NODE_EXPLORATION_SCORE] Node %03u explore score: %lf = %lf * sqrt(2 * log(%lf) / %lf)",
//              node_data->id,
//              RHO * sqrt(2 * log((double) parent_data->selected) / (double) node_data->selected),
//              RHO, (double) parent_data->selected, (double) node_data->selected);

    return  RHO * sqrt(2 * log((double) parent_data->selected) / (double) node_data->selected);
}

double seed_exploration_score(TreeNode* tree_node, int seed_index)
{
    seed_info_t* target_seed = get_tree_node_data(tree_node)->seeds[seed_index];
    TreeNodeData* node_data = get_tree_node_data(tree_node);
    if (!target_seed->selected) { return INFINITY; }
    log_trace("[SEED_EXPLORATION_SCORE] %lf = %lf * sqrt(2 * log(%lf) / %lf)",
              RHO * sqrt(2 * log((double)node_data->selected)/target_seed->selected),
              RHO, (double)node_data->selected, (double) target_seed->selected);
    return RHO * sqrt(2 * log((double)node_data->selected)/ (double) target_seed->selected);
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

double seed_score(TreeNode* tree_node, u32 seed_index)
{
    // return g_rand_int(RANDOM_NUMBER_GENERATOR);

    if (SCORE_FUNCTION == Random) return g_rand_int(RANDOM_NUMBER_GENERATOR);

//    seed_info_t* target_seed = get_tree_node_data(tree_node)->seeds[seed_index];
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

    char* message = tree_node_repr(tree_node);
    log_info("[BEST_CHILD] Selecting the best child of: %s", message);
    free(message);
    message = NULL;
    if (number_of_children == 1) {
        char* message = tree_node_repr(g_node_nth_child(tree_node, 0));
        log_info("[BEST_CHILD] Only one child: %s", message);
        free(message);
        message = NULL;
        return g_node_nth_child(tree_node, 0);
    }
    log_info("[BEST_CHILD] Multiple children");
    log_info("[BEST_CHILD] Max score: %lf; Number of ties: %u", max_score, number_of_ties);
    for (u32 child_index = 0; child_index < number_of_children; child_index++) {
        gdouble score = tree_node_score(g_node_nth_child(tree_node, child_index));
        message = tree_node_repr(g_node_nth_child(tree_node, child_index));
        log_info("[BEST_CHILD] Current index %u: %lf (%s)",
                 child_index, score, message);
        free(message);
        message = NULL;
        if (score < max_score) continue;
        if (score > max_score) number_of_ties = 0;
        max_score = score;
        ties[number_of_ties] = child_index;
        number_of_ties ++;
        log_info("[BEST_CHILD] Max score: %lf; Current score: %lf; Number of ties: %u",
                 max_score, score, number_of_ties);
        for (u32 tie_index=0; tie_index < number_of_ties; tie_index++)
        {
          char* message = tree_node_repr(g_node_nth_child(tree_node, ties[tie_index]));
          log_info("[BEST_CHILD] Tie index %u: %s",
                   tie_index, message);
          free(message);
          message = NULL;
        }
    }

    if (number_of_ties == 1)
    {
        char* message = tree_node_repr(g_node_nth_child(tree_node, ties[0]));
        log_info("[BEST_CHILD] Only one candidate: %s", message);
        free(message);
        message = NULL;
        return g_node_nth_child(tree_node, ties[0]);
    }

    log_info("[BEST_CHILD] Best child has %u candidates", number_of_ties);
    for (u32 i = 0; i < number_of_ties; ++i) {
        char* message = tree_node_repr(g_node_nth_child(tree_node, ties[i]));
        log_info("[BEST_CHILD] The %u th candidates in ties is the %u th child: %s",
                 i, ties[i], message);
        free(message);
        message = NULL;
    }
    u32 winner_index = g_rand_int_range(RANDOM_NUMBER_GENERATOR, 0, number_of_ties);
    u32 winner = ties[winner_index];
    log_info("[BEST_CHILD] Winner index in ties is: %u", winner_index);
    message = tree_node_repr(g_node_nth_child(tree_node, winner));
    log_info("[BEST_CHILD] Winner is the %u th child: %s", winner, message);
    free(message);
    message = NULL;
    return g_node_nth_child(tree_node, winner);
}

seed_info_t* best_seed(TreeNode* tree_node)
{
    TreeNodeData* tree_node_data = get_tree_node_data(tree_node);
    gdouble max_score = -INFINITY;
    u32 number_of_seeds = tree_node_data->seeds_count;
    u32 number_of_ties = 0;
    u32 ties[number_of_seeds];

    char* message = tree_node_repr(tree_node);
    log_debug("[BEST_SEED] Selecting the best seed of: %s", message);
    free(message);
    message = NULL;

    if (number_of_seeds == 1) {
        message = seed_repr(tree_node, 0, NULL);
        log_debug("[BEST_SEED] Only one seed: %s", message);
        free(message);
        message = NULL;
        return get_tree_node_data(tree_node)->seeds[0];
    }

    log_debug("[BEST_SEED] Multiple seeds");
    log_debug("[BEST_SEED] Max score: %lf; Number of ties: %u", max_score, number_of_ties);
    for (u32 seed_index = 0; seed_index < number_of_seeds; seed_index++) {
        double score = seed_score(tree_node, seed_index);

        message = seed_repr(tree_node, seed_index, NULL);
        log_debug("[BEST_SEED] Current index %u: %lf (%s)",
                 seed_index, score, message);
        free(message);
        message = NULL;
        if (score < max_score) continue;
        if (score > max_score) number_of_ties = 0;
        max_score = score;
        ties[number_of_ties] = seed_index;
        number_of_ties ++;
        log_debug("[BEST_SEED] Max score: %lf; Current score: %lf; Number of ties: %u",
                 max_score, score, number_of_ties);
        for (u32 tie_index=0; tie_index < number_of_ties; tie_index++)
        {
            message = seed_repr(tree_node, ties[tie_index], NULL);
            log_debug("[BEST_SEED] Tie index %u: %s", tie_index, message);
            free(message);
            message = NULL;
        }
    }

    if (number_of_ties == 1)
    {
        message = seed_repr(tree_node, ties[0], NULL);
        log_debug("[BEST_SEED] Only one candidate: %s", message);
        free(message);
        message = NULL;
        return get_tree_node_data(tree_node)->seeds[ties[0]];
    }

    log_debug("[BEST_SEED] Best seed has %u candidates", number_of_ties);
    for (u32 i = 0; i < number_of_ties; ++i) {
        message = seed_repr(tree_node, ties[i], NULL);
        log_debug("[BEST_SEED] The %u th candidates in ties is the %u th seed: %s",
               i, ties[i], message);
        free(message);
        message = NULL;
    }

    u32 winner_index = g_rand_int_range(RANDOM_NUMBER_GENERATOR, 0, number_of_ties);
    u32 winner = ties[winner_index];
    log_debug("[BEST_SEED] Winner index in ties is: %u", winner_index);
    message = seed_repr(tree_node, winner, NULL);
    log_debug("[BEST_SEED] Winner is the %u th seed: %s", winner, message);
    free(message);
    message = NULL;

    return get_tree_node_data(tree_node)->seeds[winner];
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

//void print_reversed_path(TreeNode* tree_node)
//{
//    do {
//        g_printf("%u ", get_tree_node_data(tree_node)->id);
//        tree_node = tree_node->parent;
//    } while (tree_node);
//    g_printf("\n");
//}
//
//void print_path(TreeNode* tree_node)
//{
//  TreeNodeData* tree_node_data = get_tree_node_data(tree_node);
//  for (u32 i = 0; i < tree_node_data->path_len; i++) {
//      g_printf("%u ", tree_node_data->path[i]);
//  }
//  g_print("\n");
//}

char* node_path_str(TreeNode* tree_node)
{
  TreeNodeData* tree_node_data = get_tree_node_data(tree_node);

    char* a_str = NULL;
    int a_str_len = asprintf(&a_str, "%u", tree_node_data->path[0]);

    for (int i = 1; i < tree_node_data->path_len; ++i) {
      log_assert(a_str_len != -1, "[NODE_PATH_STR] Function asprintf returned error code -1");
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
    log_assert(colour_code, "[COLOUR_ENCODER] Colour code was not set, incorrect colour enum? %d", colour);
    return colour_code;
}

char* seed_repr(TreeNode* tree_node, uint seed_index, seed_info_t* seed)
{
  if (!seed)  {seed = get_tree_node_data(tree_node)->seeds[seed_index];}
  else {log_assert(seed == get_tree_node_data(tree_node)->seeds[seed_index],
                   "[SEED_REPR] Param seed does not match with the seed at seed_index in tree_node: %s != %s",
                   seed->q->fname, get_tree_node_data(tree_node)->seeds[seed_index]->q->fname);}
  char* message = NULL;
  struct queue_entry* queue_entry = seed->q;
  message_append(&message,
                 "Seed %03u: %06.2lf [%06.2lf + %06.2lf] {%03u, %03u} (%s)",
                 seed->parent_index, seed_score(tree_node, seed_index),
                 seed_exploitation_score(tree_node, seed_index),
                 seed_exploration_score(tree_node, seed_index),
                 seed->selected, seed->discovered, queue_entry->fname);
  return message;
}

char* tree_node_repr(TreeNode* tree_node)
{
  TreeNodeData* tree_node_data = get_tree_node_data(tree_node);
  char* message = NULL;

  message_append(&message, "\033[1;%dm%03u: %06.2lf "
                           "[%06.2lf + %06.2lf] "
                           "{%03u, %03u} "
                           "<%03u>\033[0m",
           colour_encoder(tree_node_data->colour),
           tree_node_data->id,
           tree_node_score(tree_node),
           tree_node_exploitation_score(tree_node),
           tree_node_exploration_score(tree_node),
           tree_node_data->selected,
           tree_node_data->discovered,
           tree_node_data->seeds_count);
  return message;
}

char* region_state_repr(region_t region)
{
  char* message = NULL;
  char* message_arr = u32_array_to_str(region.state_sequence, region.state_count);
  message_append(&message, "%02u states: %s",
                 region.state_count, message_arr);
  free(message_arr);
  message_arr = NULL;
  return message;
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
    free(message);
    message = NULL;
    return;
  }
  char* message_tree_node_repr_tree_node = tree_node_repr(tree_node);
  message_append(&message, message_tree_node_repr_tree_node);
  free(message_tree_node_repr_tree_node);
  message_tree_node_repr_tree_node = NULL;
  if (tree_node == mark_node && found >= 0) message_append(&message, "\033[1;32m <=< found %u\033[0m", found);
  log_info(message);
  free(message);
  message = NULL;

  if (g_node_n_children(tree_node)) indent++;
  for (int i = 0; i < g_node_n_children(tree_node); ++i) {
    tree_log(g_node_nth_child(tree_node, i), mark_node, indent, found);
  }
}

void seed_log(TreeNode* tree_node, seed_info_t* seed_selected, char* calling_function)
{
  char* message = NULL;
  TreeNodeData* tree_node_data = get_tree_node_data(tree_node);
  for (uint i = 0; i < tree_node_data->seeds_count; ++i) {
    message = NULL;
    seed_info_t* seed = tree_node_data->seeds[i];
    message_append(&message, calling_function);
    message_append(&message, "Candidate ");
    char* message_seed_repr = seed_repr(tree_node, i, seed);
    message_append(&message, message_seed_repr);
    free(message_seed_repr);
    message_seed_repr = NULL;
    log_info(message);
    free(message);
    message = NULL;
  }
}

void seed_selected_log(TreeNode* tree_node, seed_info_t* seed_selected, char* calling_function)
{
  char* message = tree_node_repr(tree_node);
  log_assert(seed_selected != NULL,
             "[SEED_SELECTED_LOG] Seed selected cannot be NULL: %s of %s", seed_selected, message);
  free(message);
  message = NULL;
  TreeNodeData* tree_node_data = get_tree_node_data(tree_node);
  for (uint i = 0; i < tree_node_data->seeds_count; ++i) {
    seed_info_t* seed = tree_node_data->seeds[i];
    if (seed != seed_selected) {continue;}
    message = NULL;
    message_append(&message, calling_function);
    message_append(&message, "Selected ");
    char* message_seed_repr = seed_repr(tree_node, i, seed);
    message_append(&message, message_seed_repr);
    free(message_seed_repr);
    message_seed_repr = NULL;
    message_append(&message, "\033[1;32m <=< Selected\033[0m");
    log_info(message);
    free(message);
    message = NULL;
  }
}

void queue_state_log(struct queue_entry* queue)
{
  char* message = NULL;
  char* message_region_state_repr = NULL;
  for (u32 region_index = 0; region_index < queue->region_count; ++region_index) {
    message = NULL;
    message_region_state_repr = NULL;
    message_region_state_repr = region_state_repr(queue->regions[region_index]);
    message_append(&message, "Region %2u has %s", region_index, message_region_state_repr);
    free(message_region_state_repr);
    message_region_state_repr = NULL;
    if (queue->regions[region_index].state_sequence) { log_info(message); }
    else { log_warn(message); }
    free(message);
    message = NULL;
  }
}

seed_info_t* construct_seed_with_queue_entry(struct queue_entry* q)
{
  seed_info_t* seed = (seed_info_t*) malloc (sizeof(seed_info_t));
  seed->q = q;
  seed->selected = 0;
  seed->discovered = 0;
  return seed;
}

void add_seed_to_node(seed_info_t* seed, u32 matching_region_index, TreeNode * node)
{
  log_assert(seed != NULL, "[ADD_SEED_TO_NODE] Seed does not exist");
  char* message = tree_node_repr(node);
  log_assert(get_tree_node_data(node)->colour == Golden, "[ADD_SEED_TO_NODE] Not a SimNode: %s", message);
  free(message);
  message = NULL;
  /*NOTE: Figure out M2 at here so that we don't have to do it repeatedly when the same queue_entry is selected*/
//  struct queue_entry* q = seed->q;
//  region_t* regions = q->regions;
  TreeNodeData* tree_node_data = get_tree_node_data(node);

  // TOASK: Should we allocate more spaces to avoid repeated reallocation?
  TreeNodeData* node_data = get_tree_node_data(node);
  node_data->seeds = (seed_info_t **) realloc (node_data->seeds, (node_data->seeds_count + 1) * sizeof(void *));
  node_data->seeds[node_data->seeds_count] = (seed_info_t *) seed;
  node_data->region_indices = (u32*) realloc (node_data->region_indices, (node_data->seeds_count + 1) * sizeof(u32));
  node_data->region_indices[node_data->seeds_count] = matching_region_index;
  seed->parent_index = node_data->seeds_count;
  node_data->seeds_count++;

  seed_info_t* node_seed = node_data->seeds[node_data->seeds_count-1];
  struct queue_entry* node_q = node_seed->q;
  region_t node_region = node_q->regions[node_data->region_indices[seed->parent_index]];

  log_debug("[ADD_SEED_TO_NODE] Region ID of node    : %u", node_data->region_indices[node_seed->parent_index]);
  log_debug("[ADD_SEED_TO_NODE] Region count of queue: %u", node_q->region_count);
  log_assert(node_data->region_indices[node_seed->parent_index] <= node_q->region_count,
             "[ADD_SEED_TO_NODE] The region index stored in the parent node is greater than the region count: %u > %u",
             node_data->region_indices[node_seed->parent_index], node_q->region_count);

  log_debug("[ADD_SEED_TO_NODE] Region state count: %u", node_region.state_count);
  log_debug("[ADD_SEED_TO_NODE] Node path length  : %u", tree_node_data->path_len);

  message = u32_array_to_str(node_region.state_sequence, node_region.state_count);
  log_debug("[ADD_SEED_TO_NODE] Region states: %s", message);
  free(message);
  message = NULL;
  message = u32_array_to_str(tree_node_data->path, tree_node_data->path_len);
  log_debug("[ADD_SEED_TO_NODE] Node path    : %s", message);
  free(message);
  message = NULL;
  log_assert(node_region.state_count >= tree_node_data->path_len,
             "[ADD_SEED_TO_NODE] The path len of a tree node is smaller than "
             "the state count of the matching region: %u < %u",
             tree_node_data->path_len, node_region.state_count);
  message = u32_array_to_str(node_region.state_sequence, tree_node_data->path_len);
  char* message_node = u32_array_to_str(tree_node_data->path, tree_node_data->path_len);
  log_assert(!memcmp(node_region.state_sequence, tree_node_data->path, tree_node_data->path_len),
             "[ADD_SEED_TO_NODE] The path of a tree node is different to "
             "the states of the matching region:\nREGN: %s\nNODE: %s", message, message_node);
  free(message);
  free(message_node);
  message = NULL;
  message_node = NULL;
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
  char* message1 = NULL;
  char* message2 = NULL;
  while (get_tree_node_data(parent_tree_node)->colour != Golden) {
    parent_tree_node = best_child(parent_tree_node);
    message1 = tree_node_repr(parent_tree_node);
    log_info("[SELECT_TREE_NODE] Best child is: %s", message1);
    free(message1);
    message1 = NULL;
    /* NOTE: Selected stats propagation of nodes along the selection path is done here */
    get_tree_node_data(parent_tree_node)->selected++;
    /*NOTE: If the score of the best child is -inf,
     * then the score of the parent of the best child should be -inf
     * e.g. parent is a black node, whose only child is a leaf
     */
    message1 = tree_node_repr(parent_tree_node);
    if (!G_NODE_IS_ROOT(parent_tree_node)) {message2 = tree_node_repr(parent_tree_node);}
    log_assert(
            tree_node_score(parent_tree_node) > -INFINITY ||
            (!G_NODE_IS_ROOT(parent_tree_node) && get_tree_node_data(parent_tree_node->parent)->colour == Black),
            "[SELECT_TREE_NODE] If the score of the best child is -inf, "
            "then the score of the parent of the best child should be -inf:"
            "e.g. parent is a black node, whose only child is a leaf.\nNode  :%s\nParent:%s",
            message1,
            G_NODE_IS_ROOT(parent_tree_node)?"Is ROOT":message2);
    free(message1);
    free(message2);
    message1 = NULL;
    message2 = NULL;
    while (tree_node_score(parent_tree_node) == -INFINITY)
    {
      message1 = tree_node_repr(parent_tree_node);
      log_info("[SELECT_TREE_NODE] The score of the best child is -inf: %s", message1);
      free(message1);
      message1 = NULL;
      /* NOTE: If the score of the root is -inf, then ROOT must be fully explored,
       *  hence there is no point continuing;
       */
      message1 = tree_node_repr(parent_tree_node);
      log_assert(!G_NODE_IS_ROOT(parent_tree_node),
                 "[SELECT_TREE_NODE] If the score of the root is -inf, then ROOT must be fully explored, "
                 "hence there is no point continuing;\nNode: %s", message1);
      free(message1);
      message1 = NULL;
      parent_tree_node = parent_tree_node->parent;
      message1 = tree_node_repr(parent_tree_node);
      log_info("[SELECT_TREE_NODE] Checking its parent: %s", message1);
      free(message1);
      message1 = NULL;
      if (!is_fully_explored(parent_tree_node)) {
        message1 = tree_node_repr(parent_tree_node);
        log_info("[SELECT_TREE_NODE] Resume selection from its parent: %s", message1);
        free(message1);
        message1 = NULL;
        break;
      }
      get_tree_node_data(parent_tree_node)->fully_explored = TRUE;
      message1 = tree_node_repr(parent_tree_node);
      log_info("[SELECT_TREE_NODE] Setting its parent fully explored: %s", message1);
      free(message1);
      message1 = NULL;
    }
    log_assert(parent_tree_node != NULL, "[SELECT_TREE_NODE] NULL is the best child");
    message1 = tree_node_repr(parent_tree_node);
    log_assert(tree_node_score(parent_tree_node) > -INFINITY,
               "[SELECT_TREE_NODE] -inf is the score of the best child %s", message1);
    free(message1);
    message1 = NULL;
  }
  return parent_tree_node;
}

gboolean is_fully_explored(TreeNode* parent_tree_node)
{
  /*NOTE: There are four possibilities:
   * 1. White parent + white children:
   *  By definition, the parent is not fully explored even if all children are.
   *  Because there might be missing children that can be found by simulating from parent's SimNode
   * 2. White parent + Black children:
   *  Same above, as being a black child only means all existing inputs bound the black child with a grandchild
   *  (i.e. by making the server returns more than response code as the same time.)
   *  It has nothing to do with the parent node
   * 3. Black parent + white child:
   *  This is case we care about.
   *  As far as we know, it happens when the white child is a leaf. There could be other corner cases that cause this.
   *  Here I use -inf to represent "being a leaf" or "fully explored", in case of other corner cases.
   *  But the implication is the same:
   *   a) The white child must be the only child;
   *   b) The parent does not have any SimNode;
   *   c) The black parent should not be selected, otherwise we will fuzz a leaf, which is problematic and unnecessary.
   * 4. Black parent + black child:
   *  This should be the same as 3, except the black child is not a leaf.
   *  The black child might have a single white leaf.
   *  After marking the black child fully explored in the past, this will make the black parent fully explored, too.
   *  Again:
   *   a) the black child must be the only child;
   *   b) The parent does not have any SimNode;
   *   c) The black parent should not be selected, otherwise we will fuzz a leaf, which is problematic and unnecessary.
   * In short:
   *  a) If the parent is not black, it is not fully explored, return False;
   *  b) If the parent is black, it is fully explored only if the score of its only child is -inf.
   */
  if (get_tree_node_data(parent_tree_node)->fully_explored) {return TRUE;}
  if (get_tree_node_data(parent_tree_node)->colour != Black) {return FALSE;}

  char* message = tree_node_repr(parent_tree_node);
  log_assert(g_node_n_children(parent_tree_node) == 1,
             "[IS_FULLY_EXPLORED] More than one child exists under %s", message);
  log_assert(get_tree_node_data(g_node_nth_child(parent_tree_node, 0))->colour != Golden,
             "[IS_FULLY_EXPLORED] A SimNode is the only child of %s", message);
  free(message);
  message = NULL;
  return tree_node_score(g_node_nth_child(parent_tree_node, 0)) == -INFINITY;
}

seed_info_t* select_seed(TreeNode* tree_node_selected)
{
    seed_info_t* seed = best_seed(tree_node_selected);
    /* NOTE: Selected stats propagation of the seed is done here */
    seed->selected++;
    return seed;
}


TreeNode* Initialisation(uint log_lvl, uint tree_dp, uint ign_ast, double rho)
{
    char log_file[100];
    snprintf(log_file, sizeof(log_file), "%s", getenv("AFLNET_LEGION_LOG"));
    log_add_fp(fopen(log_file, "w+"), log_lvl);
    log_set_quiet(TRUE);
    set_ignore_assertion(ign_ast);
    log_info("[INITIALISATION] LOG PATH: %s", log_file);
    MAX_TREE_LOG_DEPTH = tree_dp;
    RHO = rho;
    log_info("[INITIALISATION] Log level: %u", log_lvl);
    log_info("[INITIALISATION] Max tree log depth: %u", MAX_TREE_LOG_DEPTH);
    log_info("[INITIALISATION] Ignore assertions: %s", ign_ast?"True":"False");
    log_info("[INITIALISATION] Rho: %lf", RHO);

    khmn_nodes = kh_init_hmn();
    u32 path[] = {0};
    TreeNode* root = new_tree_node(new_tree_node_data(0, White, path, 1));
    get_tree_node_data(root)->simulation_child = append_child(root, 999, Golden, path, 1);

    return root;
}

seed_info_t* Selection(TreeNode** tree_node)
{
    ROUND += 1;
    TreeNode* root = *tree_node;
    log_info("[SELECTION] ==========< ROUND %03d >==========", ROUND);
    log_assert(G_NODE_IS_ROOT(*tree_node),
               "[SELECTION] Selection does not start from ROOT, but from %s", tree_node_repr(*tree_node));

    *tree_node = select_tree_node(*tree_node);
    char* message = tree_node_repr(*tree_node);
    log_info("[SELECTION] Selected node   : %s", message);
    free(message);
    message = NULL;
    message = tree_node_repr((*tree_node)->parent);
    log_info("[SELECTION] Selected parent : %s", message);
    free(message);
    message = NULL;

    while (tree_node_score((*tree_node)->parent) == -INFINITY) {
      log_info("[SELECTION] The score of the parent of the simulation child is -inf, redo selection");
      message = tree_node_repr(*tree_node);
      log_assert(G_NODE_IS_ROOT(*tree_node),
                 "[SELECTION] Selection does not re-start from ROOT, but from %s", message);
      free(message);
      message = NULL;
      *tree_node = select_tree_node(root);
      message = tree_node_repr(*tree_node);
      log_info("[SELECTION] Selected node   : %s", message);
      free(message);
      message = NULL;
      message = tree_node_repr((*tree_node)->parent);
      log_info("[SELECTION] Selected parent : %s", message);
      free(message);
      message = NULL;
    }

    message = tree_node_repr((*tree_node)->parent);
    log_assert(tree_node_score((*tree_node)->parent) > -INFINITY,
               "[SELECTION] Selected SimNode's parent has -inf score: %s", message);
    free(message);
    message = NULL;
    seed_log(*tree_node, NULL, "[SELECTION] ");

    TreeNodeData* parent_node_data = get_tree_node_data((*tree_node)->parent);
    khiter_t k = kh_get_hmn(khmn_nodes, parent_node_data->id);
    assert(kh_exist(khmn_nodes, k));
    int count = kh_value(khmn_nodes, k);
    kh_value(khmn_nodes, k) = 1 + count;

    seed_info_t* seed_selected = select_seed(*tree_node);
    seed_selected_log(*tree_node, seed_selected, "[SELECTION] ");
    struct queue_entry* q = seed_selected->q;

    log_info("[SELECTION] Selection seed  : %s", q->fname);
    for (int i = 0; i < q->region_count; ++i) {
      message = u32_array_to_str(q->regions[i].state_sequence, q->regions[i].state_count);
      log_debug("[SELECTION] Seed region %2d  : %s",
               i, message);
      free(message);
      message = NULL;
    }
    TreeNodeData* tree_node_data = get_tree_node_data(*tree_node);
    log_info("[SELECTION] Selection path    : %s", node_path_str(*tree_node));
    message = u32_array_to_str(q->regions[tree_node_data->region_indices[seed_selected->parent_index]].state_sequence,
                               q->regions[tree_node_data->region_indices[seed_selected->parent_index]].state_count);
    log_debug("[SELECTION] Selection region  : %s", message);
    free(message);
    message = NULL;
    return seed_selected;
}

char* Simulation(TreeNode* target)
{
    assert(get_tree_node_data(target)->colour == Golden);
    return NULL;
}

void preprocess_queue_entry(struct queue_entry* q)
{
  u32 null_region_count = 0;
  char* message = NULL;
  for (u32 region_index = 0; region_index < q->region_count; ++region_index) {
    message = u32_array_to_str(q->regions[region_index].state_sequence, q->regions[region_index].state_count);
    log_assert((q->regions[region_index].state_count > 0) == (q->regions[region_index].state_sequence != NULL),
               "The state count %u does not match with state sequence:\n%s",
               q->regions[region_index].state_count, message);
    free(message);
    message = NULL;
    if (!null_region_count && q->regions[region_index].state_sequence != NULL) {continue;}
    while (region_index+null_region_count < q->region_count
           && q->regions[region_index+null_region_count].state_sequence == NULL)
    {
      null_region_count++;
    }
    if (region_index+null_region_count >= q->region_count)
    {
      log_info("[PREPROCESS_QUEUE_ENTRY] Reached region count %u while searching for region at index %u",
               q->region_count, region_index+null_region_count);
      q->region_count = region_index;
      log_info("[PREPROCESS_QUEUE_ENTRY] Set region count to be %u accordingly", q->region_count);
      break;
    }
    log_info("[PREPROCESS_QUEUE_ENTRY] Replacing region ID %u with %u", region_index, region_index+null_region_count);
    message = u32_array_to_str(q->regions[region_index+null_region_count].state_sequence,
                               q->regions[region_index+null_region_count].state_count);
    log_assert(q->regions[region_index+null_region_count].state_sequence != NULL,
               "State sequence at region index %u should not be NULL: %s",
               region_index+null_region_count, message);
    free(message);
    message = NULL;
    q->regions[region_index] = q->regions[region_index+null_region_count];
  }

  for (u32 region_index = 0; region_index < q->region_count; ++region_index)
  {
    message = u32_array_to_str(q->regions[region_index].state_sequence, q->regions[region_index].state_count);
    log_assert(q->regions[region_index].state_sequence != NULL && q->regions[region_index].state_count,
               "After preprocessing q, region %u has %u states: %s",
               region_index,
               q->regions[region_index].state_count,
               message);
    free(message);
    message = NULL;
  }
}


TreeNode* Expansion(TreeNode* tree_node, struct queue_entry* q, u32* response_codes, u32 len_codes, gboolean* is_new)
{
  TreeNode* parent_node;
  *is_new = FALSE;
  u32 matching_region_index = 0;
  gboolean matched_exactly = FALSE;
  char* message = NULL;
  char* message_node = NULL;
  char* message_parent = NULL;

  // Construct seed with queue_entry q
  seed_info_t* seed = NULL;

  log_info("[MCTS-EXPANSION] Starts");
  log_info("[MCTS-EXPANSION] Record Queue Entry: %s", q->fname);
  message = u32_array_to_str(response_codes, len_codes);
  log_info("[MCTS-EXPANSION] State seq has %02u states: %s", len_codes, message);
  free(message);
  message = NULL;
  message = u32_array_to_str(q->regions[q->region_count-1].state_sequence,
                             q->regions[q->region_count-1].state_count);
  log_info("[MCTS-EXPANSION] Last reg  has %02u states: %s",
           q->regions[q->region_count-1].state_count, message);
  free(message);
  message = NULL;

  log_assert(q->regions[q->region_count-1].state_count == len_codes,
             "State count in the last region (%02u) != response code len (%02u)",
             q->regions[q->region_count-1].state_count, len_codes);
  message = u32_array_to_str(response_codes, len_codes);
  message_node = u32_array_to_str(q->regions[q->region_count-1].state_sequence, len_codes);
  log_assert(!memcmp(q->regions[q->region_count-1].state_sequence, response_codes, len_codes),
             "Response codes are not the same as the state sequences in the last region of q:\nRSP: %s\nRGN: %s",
             message, message_node);
  free(message);
  free(message_node);
  message = NULL;
  message_node = NULL;

  // Check if the response code sequence is new
  // And add the new queue entry to each node along the paths
  assert(response_codes[0] == 0);
  log_assert(response_codes[0] == 0, "[MCTS-EXPANSION] Response codes sequence does not start with 0");
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

    for (u32 region_index = matching_region_index + matched_exactly;
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
      message = u32_array_to_str(response_codes, path_index+1);
      log_debug("[MCTS-EXPANSION] Matching code %03u at index %3u: %s",
               response_codes[path_index], path_index, message);
      free(message);
      message = NULL;
      message = u32_array_to_str(region.state_sequence, region.state_count);
      log_debug("[MCTS-EXPANSION] With region at index %3u      : %s (Queue Entry %s)",
                region_index, message, q->fname);
      free(message);
      message = NULL;
      if (path_index+1 <= region.state_count) {
//        exact_match = (path_index+1 == region.state_count);
//        matched_exactly = response_codes[path_index] == region.state_sequence[region.state_count-1];
        gboolean matched_len = path_index == region.state_count-1;
        gboolean matched_sequence = memcmp(response_codes, region.state_sequence, region.state_count) == 0;
        matched_exactly = matched_len && matched_sequence;
        matching_region_index = region_index;
        if (matched_exactly) {log_debug("[MCTS-EXPANSION] Exact Match found");}
        else  {log_debug("[MCTS-EXPANSION] Partial Match found");}
        break;
      }
    }

    if (*is_new)  {
      enum node_colour colour;
      if (matched_exactly)  {colour = White;}
      else  {colour = Black;}
      tree_node = append_child(parent_node, response_codes[path_index], colour, response_codes, path_index+1);

      message_node = tree_node_repr(tree_node);
      message_parent = tree_node_repr(parent_node);
      log_debug("[MCTS-EXPANSION] Add a new child %s of parent %s", message_node, message_parent);
      free(message_node);
      free(message_parent);
      message_node = NULL;
      message_parent = NULL;
    }
    if (matched_exactly) {
      TreeNodeData* tree_node_data = get_tree_node_data(tree_node);
      if (tree_node_data->colour == Black && path_index + 1 != len_codes) {
        // NOTE: Flip a node if this is black and is not an termination point
        //  But still was considered as Black
        message_node = tree_node_repr(tree_node);
        log_debug("[MCTS-EXPANSION] Flipping node from Black to White: %s", message_node);
        free(message_node);
        message_node = NULL;
//        tree_log(ROOT, tree_node, 0, 0);
        tree_node_data->colour = White;
        tree_node_data->simulation_child = append_child(tree_node, 999, Golden, response_codes, path_index+1);
        message_node = tree_node_repr(tree_node);
        log_debug("[MCTS-EXPANSION] Flipped node: %s", message_node);
        free(message_node);
        message_node = NULL;

//        tree_log(ROOT, tree_node, 0, 0);
      }
      if (tree_node_data->colour == White && q->regions[matching_region_index].state_count < len_codes) {
        //NOTE: Only add seed to node if node colour is White  (Otherwise there is no simulation child)
        // and the matching region is not the last region in the q, (otherwise M2 count is 0)
        seed = construct_seed_with_queue_entry(q);
        add_seed_to_node(seed, matching_region_index, get_simulation_child(tree_node));
        TreeNodeData* sim_data = get_tree_node_data(get_simulation_child(tree_node));
        seed_info_t* seed = sim_data->seeds[sim_data->seeds_count-1];
//        struct queue_entry* new_q = (struct queue_entry*) seed->q;
        region_t region = q->regions[matching_region_index];
        log_debug("[MCTS-EXPANSION] The following two paths should match");
        message = u32_array_to_str(region.state_sequence, region.state_count);
        log_debug("[MCTS-EXPANSION] Region %d: %s",
                 matching_region_index, message);
        free(message);
        message = NULL;
        message = u32_array_to_str(tree_node_data->path, tree_node_data->path_len);
        log_debug("[MCTS-EXPANSION] Node %d: %s",
                 tree_node_data->id, message);
        free(message);
        message = NULL;
        log_assert(region.state_count >= tree_node_data->path_len,
                   "[MCTS-EXPANSION] The number of states in region is smaller than "
                   "the number of states in the node path: %u < %u",
                   region.state_count, tree_node_data->path);
        message = u32_array_to_str(region.state_sequence, tree_node_data->path_len);
        message_node = u32_array_to_str(tree_node_data->path, tree_node_data->path_len);
        log_assert(!memcmp(region.state_sequence, tree_node_data->path, sizeof(u32)*tree_node_data->path_len),
                   "[MCTS-EXPANSION] The region states sequence is different to "
                   "the node states sequence:\nREGN: %s\nNODE: %s",
                   message, message_node);
        free(message);
        free(message_node);
        message = NULL;
        message_node = NULL;
        message_node = tree_node_repr(tree_node);
        log_debug("[MCTS-EXPANSION] Seed appended at index %u of the simulation child of node %s , with region id %u",
                 sim_data->seeds_count-1, message_node,
                 sim_data->region_indices[seed->parent_index]);
        free(message_node);
        message_node = NULL;
      }
    }
    TreeNodeData* tree_node_data = get_tree_node_data(tree_node);
    message_node = tree_node_repr(tree_node);
    message = u32_array_to_str(tree_node_data->path, tree_node_data->path_len);
    log_debug("[MCTS-EXPANSION] Node %s path:%s", message_node, message);
    free(message_node);
    free(message);
    message_node = NULL;
    message = NULL;

      /* NOTE: Assert the path of each node is saved correctly */
      log_assert(tree_node_data->path_len == path_index+1,
                 "[MCTS-EXPANSION] Node path len is different to the current index (of response codes seq) + 1: "
                 "%u != %u",
                 tree_node_data->path_len, path_index + 1);
      message = u32_array_to_str(tree_node_data->path, tree_node_data->path_len);
      message_node = u32_array_to_str(response_codes, tree_node_data->path_len);
      log_assert(!memcmp(tree_node_data->path, response_codes, sizeof(u32)*tree_node_data->path_len),
                 "[MCTS-EXPANSION] Node path is different to the the current prefix seq of response codes:\n"
                 "RESP: %s\nNODE: %s",
                 message, message_node);
      free(message);
      free(message_node);
      message = NULL;
      message_node = NULL;
  }
  parent_node = tree_node;
  message_node = tree_node_repr(tree_node);
  log_debug("[MCTS-EXPANSION] Node: %s", message_node);
  free(message_node);
  message_node = NULL;

  message = u32_array_to_str(get_tree_node_data(tree_node)->path, get_tree_node_data(tree_node)->path_len);
  log_debug("[MCTS-EXPANSION] Node's path:%s", message);
  free(message);
  message = NULL;
  message = u32_array_to_str(response_codes, len_codes);
  log_debug("[MCTS-EXPANSION] Exec's path:%s", message);
  free(message);
  message = NULL;
  get_tree_node_data(tree_node)->fully_explored = is_leaf(tree_node);

  message_node = tree_node_repr(tree_node);
  log_debug("[MCTS-EXPANSION] Node: %s", message_node);
  free(message_node);
  message_node = NULL;
  if (G_NODE_IS_ROOT(tree_node->parent)) {
    log_debug("[MCTS-EXPANSION] Reached ROOT;");
  }

  /*NOTE: Stats propagation along the execution path is done here*/
  while (parent_node) {
      get_tree_node_data(parent_node)->discovered += *is_new;
      parent_node = parent_node->parent;
  }

//  /* NOTE: Stats propagation of the seed is done here */
//  seed->discovered += *is_new;
//  seed->selected += 1;
  return tree_node;
}

void Propagation(TreeNode* leaf_selected, seed_info_t* seed_selected, gboolean is_new)
{
  char* message = tree_node_repr(leaf_selected);
  log_info("[PROPAGATION] Back-propagating from SimNode: %s", message);
  free(message);
  message = NULL;
  message = tree_node_repr(leaf_selected->parent);
  log_info("[PROPAGATION] The parent of the SimNode is : %s", message);
  free(message);
  message = NULL;

  if (ROUND <= 0) {
    /* NOTE: Temporarily only propagate stats to SimNote during normal run
//     *  Tried disable propagation in both dry-run and 1st run,
//     *  did not work
     */
    /*  NOTE: When the propagation logic changes, check this as well */

    log_info("[PROPAGATION] Skipping propagations of the SimNode in round %03u", ROUND);
    return;
  }

  TreeNode* leaf_parent = leaf_selected;
  TreeNodeData* tree_node_data = get_tree_node_data(leaf_parent);
  tree_node_data->discovered += is_new;

  /* NOTE: Stats propagation of the seed is done here */
  seed_selected->discovered += is_new;

//  while (leaf_parent) {
//    TreeNodeData* tree_node_data = get_tree_node_data(leaf_parent);
//    tree_node_data->selected += 1;
//    log_info("[PROPAGATION] Back-propagated: %s", tree_node_repr(leaf_parent));
//    leaf_parent = leaf_parent->parent;
//  }
//  seed_selected->discovered += is_new;
//  seed_selected->selected += 1;

//  tree_log(ROOT, leaf_selected, 0, is_new);
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

//void preprocess_response_codes(struct queue_entry* q, u32* response_codes, u32* len_codes)
//{
//  *len_codes = q->regions[q->region_count-1].state_count;
//  log_assert(!memcmp(q->regions[q->region_count-1].state_sequence, response_codes, *len_codes),
//             "Response codes are not the same as the state sequences in the last region of q:\nRSP: %s\nRGN: %s",
//             u32_array_to_str(response_codes, *len_codes),
//             u32_array_to_str(q->regions[q->region_count-1].state_sequence, *len_codes));
//}
//
//void preprocess_simulation_results(struct queue_entry* q, u32* response_codes, u32* len_codes)
//{
//  preprocess_queue_entry(q);
//  preprocess_response_codes(q, response_codes, len_codes);
//}
/* EOF */
