#include "TreeNode.h"


TreeNodeData * new_tree_node_data (gchar * response_code, enum node_colour colour, const gchar * input_prefix)
{
    TreeNodeData * tree_node_data;
    tree_node_data = g_new (TreeNodeData, 1); //  allocates 1 element of TreeNode

    // set property
    tree_node_data->response_code = response_code;
    //TODO: This is probably not needed, left it here in case it comes in handy later.
    tree_node_data->colour = colour;
    //TODO: Maybe some response codes indicate terminations (i.e. leaves) so that they can be fully explored states?
    tree_node_data->fully_explored = FALSE;
    //TODO: Is there a way for us to know if we have found all possible inputs to fuzz a node?
    tree_node_data->exhausted = FALSE;

    // set input prefix
    tree_node_data->input_prefix = g_strdup(input_prefix);

    // set statistics
    tree_node_data->score = INFINITY;
    tree_node_data->sel_try = 0;
    tree_node_data->sel_win = 0;
    tree_node_data->sim_try = 0;
    tree_node_data->sim_win = 0;

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


double compute_exploitation_score(TreeNode * tree_node)
{
    return get_tree_node_data(tree_node)->sim_win / get_tree_node_data(tree_node)->sel_try;
}


double compute_exploration_score(TreeNode * tree_node)
{
    return RHO * sqrt(2*log(get_tree_node_data(tree_node->parent)->sel_try/get_tree_node_data(tree_node)->sel_try));
}


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

    if (!get_tree_node_data(tree_node)->sel_try)  return INFINITY;

    double exploit_score = compute_exploitation_score(tree_node);
    double explore_score = compute_exploration_score(tree_node);

    return exploit_score + explore_score;
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
    /* TODO: Can we ever know for sure that a node/subtree is fully explored?
     * Fish bone optimisation: if a simulation child
     * has only one sibling X who is not fully explored,
     * and X is not white (so that all siblings are found)
     * then do not simulate from that simulation child but only from X
     * as all new paths can only come from X
     */
    gboolean is_simul = (get_tree_node_data(tree_node)->colour == Golden);

    return FALSE;
}

TreeNode * best_child(TreeNode * tree_node)
{
    gdouble max_score = -INFINITY;
    gint number_of_children = g_node_n_children(tree_node);
    gint number_of_ties = 0;
    gint ties[number_of_children];

    for (gint child_index = 0; child_index < number_of_children; child_index++) {
        int score = tree_node_score(g_node_nth_child(tree_node, child_index));
        if (score < max_score) continue;
        if (score > max_score) number_of_ties = 0;
        ties[number_of_ties++] = child_index;
    }
    return g_node_nth_child(tree_node, g_rand_int_range(RANDOM_NUMBER_GENERATOR, 0, number_of_ties));
}

char * mutate(TreeNode * tree_node)
{
    char * prefix = get_tree_node_data(tree_node)->input_prefix;
    //TODO: fill in later
    char * suffix = "";

    return  strcat(prefix, suffix);
}

TreeNode * match_child(TreeNode * tree_node, gchar * target_response_code)
{
    TreeNode * child_node = g_node_first_child(tree_node);

    while (child_node)
    {
        tree_node_print(child_node);
        if (strcmp(get_tree_node_data(child_node)->response_code,target_response_code))  return child_node;
        child_node = g_node_next_sibling(child_node);
    }
    return NULL;
}

TreeNode * append_child(TreeNode * tree_node, gchar * child_response_code, enum node_colour colour, char * input_prefix)
{
    return g_node_append_data(tree_node, new_tree_node_data(child_response_code, colour, input_prefix));
}

void print_reversed_path(TreeNode * tree_node)
{
    do {
        g_printf("%s ", get_tree_node_data(tree_node)->response_code);
        tree_node = tree_node->parent;
    }while (tree_node);
    g_printf("\n");
}

//void print_reversed_path(TreeNode * tree_node)
//{
//    char * path = malloc(0);
////    char * path = get_tree_node_data(tree_node)->response_code;
////    while (!G_NODE_IS_ROOT(tree_node))
////    {
////        tree_node = tree_node->parent;
////
////    }
//    do {
//        gchar * response_code = get_tree_node_data(tree_node)->response_code;
////        gchar * tmp = (gchar *) malloc(sizeof(path));
////        strcpy(tmp, path);
//        g_printf("Start: path: %s (%d); response_code: %s (%d);\n", path, sizeof(path), response_code, sizeof(response_code));
//        gchar * tmp = (gchar *) malloc(sizeof(path) + sizeof(response_code));
//        g_printf("Resized tmp: tmp: %s (%d); response_code: %s (%d);\n", tmp, sizeof(tmp), response_code, sizeof(response_code));
//        snprintf(tmp, sizeof(path) + sizeof(response_code), "%s%s", response_code, path);
//        g_printf("Copied tmp: tmp: %s (%d); response_code: %s (%d);\n", tmp, sizeof(tmp), response_code, sizeof(response_code));
////        free(path);
//        char * path = (gchar *) malloc(sizeof(path) + sizeof(response_code));
//        g_printf("Resize: path: %s (%d); response_code: %s (%d);\n", path, sizeof(path), response_code, sizeof(response_code));
//        snprintf(path, sizeof(tmp), "%s", tmp);
//        g_printf("Final: path: %s (%d); response_code: %s (%d);\n", path, sizeof(path), response_code, sizeof(response_code));
////        free(tmp);
////        snprintf(path, sizeof(malloc(sizeof(path) + sizeof(response_code) + 2 * !strcmp(path, ""))), "%s, %s", response_code, path);
//        tree_node = tree_node->parent;
//    }while (tree_node);
//    g_printf("%s\n", path);
//}

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
    g_printf ("\033[1;%dmres_code: %s, score: %lf\033[0m\n",
              colour_encoder(tree_node_data->colour), tree_node_data->response_code, tree_node_data->score);
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

TreeNode * Selection(TreeNode * parent_tree_node)
{
    while (get_tree_node_data(parent_tree_node)->colour != Golden) {
        parent_tree_node = best_child(parent_tree_node);
        assert(parent_tree_node);
    }
    return parent_tree_node;
}

char * Simulation(TreeNode * target)
{
    assert(get_tree_node_data(target)->colour == Golden);
    return mutate(target);
}

gboolean Expansion(TreeNode * tree_node, char ** response_codes, char ** input_prefix, TreeNode ** execution_leaf)
{
    TreeNode * child_node;
    gboolean is_new = FALSE;
    /* NOTE: the size of each element in an array is the same */
    for (int i = 0; i < sizeof(response_codes) / sizeof(response_codes[0]); i++) {
        if (tree_node = exists_child(tree_node, response_codes[i])) continue;
        is_new = TRUE;
        tree_node = append_child(tree_node, response_codes[i], White, input_prefix[i]);
    }
    execution_leaf = &tree_node;
    return is_new;
}

void Propagation(TreeNode * selection_leaf, TreeNode * execution_leaf, gboolean is_new)
{
    gboolean is_preserved = FALSE;
    while (execution_leaf)
    {
        get_tree_node_data(execution_leaf)->sim_try += 1;
        get_tree_node_data(execution_leaf)->sim_win += is_new;
        is_preserved = is_preserved || (execution_leaf == selection_leaf->parent);
        execution_leaf = execution_leaf->parent;
    }
    while (selection_leaf)
    {
        get_tree_node_data(selection_leaf)->sel_try += 1;
        get_tree_node_data(selection_leaf)->sel_win += is_preserved;
        selection_leaf = selection_leaf->parent;
    }
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
