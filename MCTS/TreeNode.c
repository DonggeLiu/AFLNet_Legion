#include "TreeNode.h"

/* ============================================== TreeNode Functions ============================================== */

TreeNodeData * new_tree_node_data (int response_code, enum node_colour colour)
{
    TreeNodeData * tree_node_data;
    tree_node_data = g_new (TreeNodeData, 1); //  allocates 1 element of TreeNode

    // set property
    tree_node_data->stats.id = response_code;
    //NOTE: This is probably not needed, left it here in case it comes in handy later.
    tree_node_data->colour = colour;
    //TODO: Detect terminations (i.e. leaves) with some response code, and mark them fully explored.
    // e.g. predefine a set of termination code
    //TOASK: Given the last node of a past communication, would it have any child if we give it more inputs?
    tree_node_data->fully_explored = FALSE;
    //NOTE: No way to know if we have found all possible inputs to fuzz a node.
    tree_node_data->exhausted = FALSE;

    // set statistics
    tree_node_data->stats.score = INFINITY;
    tree_node_data->stats.selected_times = 0;
    tree_node_data->stats.paths_discovered = 0;
    tree_node_data->stats.paths = 0;
    tree_node_data->stats.fuzzs = 0;
    tree_node_data->stats.selected_seed_index = 0;
    tree_node_data->seeds_count = 0;

    return tree_node_data;
}


TreeNode * new_tree_node(TreeNodeData * tree_data)
{
    GNode * tree_node;
    tree_node = g_node_new (tree_data);
    return tree_node;
}


TreeNodeData * get_tree_node_data(TreeNode * tree_node)
{
    return (TreeNodeData *) tree_node->data;
}


double tree_node_exploitation_score(TreeNode * tree_node)
{
    TreeNodeData * node_data = get_tree_node_data(tree_node);
//    g_printf("%u / %u = %lf\n",
//             get_tree_node_data(tree_node)->stats.paths_discovered,
//             get_tree_node_data(tree_node)->stats.selected_times,
//             (double) get_tree_node_data(tree_node)->stats.paths_discovered / get_tree_node_data(tree_node)->stats.selected_times);
    return (double) get_tree_node_data(tree_node)->stats.paths_discovered / get_tree_node_data(tree_node)->stats.selected_times;
}

/* TOASK: which variable stores the number of times each queue_entry is selected? */
//double seed_exploitation_score(TreeNode * tree_node, int seed_index)
//{
//    struct queue_entry * target_seed = get_tree_node_data(tree_node).seeds[seed_index];
//    //TODO: replace the denominator with the number of times the seed was selected.
////    return target_seed->unique_state_count / target_seed.?
//    return 0;
//}

double tree_node_exploration_score(TreeNode * tree_node)
{
    if (G_NODE_IS_ROOT(tree_node)) return INFINITY;
    g_assert(tree_node->parent);
//    g_printf("%lf * sqrt(2*log(%u)/%u)\n",
//             RHO,
//             get_tree_node_data(tree_node->parent)->stats.selected_times,
//             get_tree_node_data(tree_node)->stats.selected_times);
    return  RHO * sqrt(2*log((double) get_tree_node_data(tree_node->parent)->stats.selected_times) / get_tree_node_data(tree_node)->stats.selected_times);
}

/* TOASK: which variable stores the number of times each queue_entry is selected? */
//double seed_exploration_score(TreeNode * tree_node, int seed_index)
//{
//    struct queue_entry * target_seed = get_tree_node_data(tree_node).seeds[seed_index];
//    //TODO: replace the denominator with the number of times the seed was selected.
////    return RHO * sqrt(2*log(get_tree_node_data(tree_node)->stats.selected_times / target_seed.?));
//    return 0;
//}

double tree_node_score(TreeNode * tree_node)
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

    if (G_NODE_IS_ROOT(tree_node))  return INFINITY;

    if (fits_fish_bone_optimisation(tree_node)) return -INFINITY;

    if (!get_tree_node_data(tree_node)->stats.selected_times)  return INFINITY;

    double exploit_score = tree_node_exploitation_score(tree_node);
    double explore_score = tree_node_exploration_score(tree_node);

    return exploit_score + explore_score;
}

double seed_score(TreeNode * tree_node, int seed_index)
{
    return g_rand_int(RANDOM_NUMBER_GENERATOR);
    if (SCORE_FUNCTION == Random) return g_rand_int(RANDOM_NUMBER_GENERATOR);

/* TOASK: which variable stores the number of times each queue_entry is selected? */
//    //TODO: replace the ? with the number of times the seed was selected.
//    if (!target_seed.?)  return INFINITY;
//
//    double exploit_score = seed_exploitation_score(tree_node, seed_index);
//    double explore_score = seed_exploration_score(tree_node, seed_index);
//
//    return exploit_score + explore_score;
}

gboolean is_fully_explored(TreeNode *  tree_node)
{
    /*
     * Check if a node is fully explored:
     *  1. If not in PERSISTENT mode, return the exact stats of the tree node
     *  2. If in PERSISTENT mode, but the root is not fully explored, return the exact stats
     *  3. If in PERSISTENT mode and the root is fully explored, but the node is root, return the exact stats
     *  4. Else a node is fully explored if it is a non-Golden leaf, or it is exhausted
     */
    TreeNodeData * tree_node_data = get_tree_node_data(tree_node);
    if (!PERSISTENT)    return tree_node_data->fully_explored;

    if (!get_tree_node_data(g_node_get_root(tree_node))->fully_explored)  return tree_node_data->fully_explored;

    if (G_NODE_IS_ROOT(tree_node))  return tree_node_data->fully_explored;

    return (is_leaf(tree_node) && tree_node_data->colour != Golden) || tree_node_data->exhausted;


}

gboolean is_leaf(TreeNode *tree_node) {
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

gboolean fits_fish_bone_optimisation(TreeNode * tree_node)
{
    /* NOTE: We might not be able to know for sure if a node/subtree is fully explored.
     *  One optimisation is to predefine a set of termination response codes.
     * Fish bone optimisation: if a simulation child
     * has only one sibling X who is not fully explored,
     * and X is not white (so that all siblings are found)
     * then do not simulate from that simulation child but only from X
     * as all new paths can only come from X
     */
//    gboolean is_simul = (get_tree_node_data(tree_node)->colour == Golden);

    return FALSE;
}

TreeNode * best_child(TreeNode * tree_node)
{
    gdouble max_score = -INFINITY;
    gint number_of_children = g_node_n_children(tree_node);
    gint number_of_ties = 0;
    gint ties[number_of_children];

    if (number_of_children == 1) {
        g_printf("Only one child: ");
        tree_node_print(g_node_nth_child(tree_node, 0));
        return g_node_nth_child(tree_node, 0);
    }

    for (gint child_index = 0; child_index < number_of_children; child_index++) {
        gdouble score = tree_node_score(g_node_nth_child(tree_node, child_index));
        g_printf("max score: %lf, current score: %lf\n", max_score, score);
        if (score < max_score) continue;
        if (score > max_score) number_of_ties = 0;
        max_score = score;
        ties[number_of_ties] = child_index;
        number_of_ties ++;
        g_printf("max score: %lf, current score: %lf, number of ties: %d, current child index: %d\n",
               max_score, score, number_of_ties, child_index);
    }

    if (number_of_ties == 1)
    {
        g_printf("Only one candidate: ");
        tree_node_print(g_node_nth_child(tree_node, ties[0]));
        return g_node_nth_child(tree_node, ties[0]);
    }

    g_printf("Best child has %d candidates: \n", number_of_ties);
    for (int i = 0; i < number_of_ties; ++i) {
        tree_node_print(g_node_nth_child(tree_node, ties[i]));
    }
    g_printf("\n");
    return g_node_nth_child(tree_node, ties[g_rand_int_range(RANDOM_NUMBER_GENERATOR, 0, number_of_ties)]);
}

struct queue_entry * best_seed(TreeNode * tree_node)
{
    gdouble max_score = -INFINITY;
    gint number_of_seeds = get_tree_node_data(tree_node)->seeds_count;
    gint number_of_ties = 0;
    gint ties[number_of_seeds];

    if (number_of_seeds == 1) {
        g_printf("Only one seed: ");
        return get_tree_node_data(tree_node)->seeds[0];
    }

    for (gint seed_index = 0; seed_index < number_of_seeds; seed_index++) {
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

    int winner_index = ties[g_rand_int_range(RANDOM_NUMBER_GENERATOR, 0, number_of_ties)];
    return get_tree_node_data(tree_node)->seeds[winner_index];
}

char * mutate(TreeNode * tree_node)
{
    //TODO: fill in later
    return "";
}

TreeNode * exists_child(TreeNode * tree_node, int target_response_code)
{
    TreeNode * child_node = g_node_first_child(tree_node);

    while (child_node)
    {
        tree_node_print(child_node);
        if (get_tree_node_data(child_node)->stats.id == target_response_code)  return child_node;
        child_node = g_node_next_sibling(child_node);
    }
    return NULL;
}

TreeNode * append_child(TreeNode * tree_node, int child_response_code, enum node_colour colour)
{
    TreeNode * child = g_node_append_data(tree_node, new_tree_node_data(child_response_code, colour));
    if (colour != Golden) get_tree_node_data(child)->simulation_child = append_child(child, -1, Golden);

    return child;
}

void print_reversed_path(TreeNode * tree_node)
{
    do {
        g_printf("%d ", get_tree_node_data(tree_node)->stats.id);
        tree_node = tree_node->parent;
    }while (tree_node);
    g_printf("\n");
}

void print_path(TreeNode * tree_node)
{
    int * path, * reversed_path;
    int path_len = 0;
    int path_size = 0;

    while (tree_node) {
        //NOTE: dynamically expanding the size of array
        if (path_len >= path_size - 1) {
            path_size += 100;
            int * new_reversed_path = ck_alloc(path_size*sizeof(int));
            if (new_reversed_path) {
                for (int i = 0; i < path_len; ++i) {
                    new_reversed_path[i] = reversed_path[i];
                }
                // TOASK: free causes error
//                ck_free(reversed_path);
//                int * reversed_path = new_reversed_path;
                reversed_path = new_reversed_path;
            }
            else {
                g_print("Insufficient Memory when printing path");
                exit(0);
            }
        }

        // collect addresses from leaf to root
        reversed_path[path_len] = get_tree_node_data(tree_node)->stats.id;
        path_len += 1;
        tree_node = tree_node->parent;
    }
    g_printf("%d of %d\n", path_len, path_size);

    for (int i = 0; i < path_len; i++) {
        g_printf("%d ", reversed_path[i]);
    }

    path = ck_alloc(path_len*sizeof(int));
    for (int i = 0; i < path_len; i++) {
        path[i] = reversed_path[path_len-i-1];
    }
    g_print("\n");
    // TOASK: free causes error
//    ck_free(reversed_path);
    for (int i = 0; i < path_len; i++) {
        g_printf("%d ", path[i]);
    }
    g_print("\n");
//    free(path);
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
            colour_code = 30;
            break;
    }
    assert(colour_code);
    return colour_code;
}

void tree_print(TreeNode * tree_node, TreeNode * mark_node, int indent, int found)
{
    char * s;
    for (int i = 0; i < indent-1; ++i) g_print("|  ");
    if (indent) g_print("|-- ");
    tree_node_print(tree_node);
//    if (tree_node == mark_node) g_printf("\\033[1;32m <=< found %d\\033[0;m",found);
    if (g_node_n_children(tree_node)) indent++;
    for (int i = 0; i < g_node_n_children(tree_node); ++i) {
        tree_print(g_node_nth_child(tree_node, i), mark_node, indent, found);
    }
}

void tree_node_print (TreeNode * tree_node)
{
    TreeNodeData * tree_node_data = get_tree_node_data(tree_node);
//    g_printf ("%d res_code: %d, score: %lf\n",
    g_printf ("\033[1;%dmres_code: %u, score: %lf (%lf + %lf) \033[0m\n",
              colour_encoder(tree_node_data->colour),
              tree_node_data->stats.id,
              tree_node_score(tree_node),
              tree_node_exploitation_score(tree_node),
              tree_node_exploration_score(tree_node));
}

//
//void
//tree_node_free (TreeNode * tree_node)
//{
//    g_free (tree_node->input_prefix); /* causes segfault*/
//    g_free (tree_node);
//}
//
//
//gboolean
//traverse_free_func (GNode * gnode, gpointer data)
//{
//    tree_node_free (TreeNode(gnode->data));
//    return FALSE;
//}

/* ============================================== TreeNode Functions ============================================== */
/* ================================================ MCTS Functions ================================================ */

TreeNode * select_tree_node(TreeNode * parent_tree_node)
{
    while (get_tree_node_data(parent_tree_node)->colour != Golden) {
        tree_node_print(parent_tree_node);
        parent_tree_node = best_child(parent_tree_node);
        /* NOTE: Stats propagation along the selection path is done here */
        get_tree_node_data(parent_tree_node)->stats.selected_times++;
        assert(parent_tree_node);
    }
    return parent_tree_node;
}


struct queue_entry * select_seed(TreeNode * tree_node_selected)
{
    return best_seed(tree_node_selected);
}


TreeNode * Initialisation()
{
    return new_tree_node(new_tree_node_data(0,White));
}

//TODO: According to afl-fuzz.c `choose_target_state`, this should return "target_state_id"
struct queue_entry * Selection(TreeNode * parent_tree_node)
{
    TreeNode * node_selected = select_tree_node(parent_tree_node);
    struct queue_entry * seed_selected = select_seed(node_selected);
    return seed_selected;
}

char * Simulation(TreeNode * target)
{
    assert(get_tree_node_data(target)->colour == Golden);
    return mutate(target);
}

TreeNode * Expansion(TreeNode * tree_node, int * response_codes, int len_codes, gboolean * is_new)
{
    TreeNode * parent_node;
    *is_new = FALSE;

    for (int i = 0; i < len_codes; i++) {
        parent_node = tree_node;
        if ((tree_node = exists_child(tree_node, response_codes[i]))) continue;
        *is_new = TRUE;
        tree_node = append_child(parent_node, response_codes[i], White);
    }

    /*NOTE: Stats propagation along the execution path is done here*/
    while (tree_node) {
        get_tree_node_data(tree_node)->stats.paths_discovered += *is_new;
        tree_node = tree_node->parent;
    }

    return tree_node;
}

void parent(TreeNode * child, TreeNode ** parent){
    /*
     * Takes a pointer to a child node, and a pointer to the pointer of a NULL
     * Then assigned the pointer to the parent of the child to the second parameter
     */
    parent = &child->parent;
}

/* ================================================ MCTS Functions ================================================ */



/* EOF */
