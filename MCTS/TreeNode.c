#include "TreeNode.h"


}TreeNode;


TreeNode *
tree_node_new (const gint response_code, enum node_colour colour, const gchar * input_prefix)
{
    TreeNode * node;
    node = g_new (TreeNode, 1); //  allocates 1 element of TreeNode

    // set property
    node->response_code = response_code;
    node->colour = colour;
    node->fully_explored = FALSE;

    // set input prefix
    node->input_prefix = g_strdup(input_prefix);

    // set statistics
    node->score = INFINITY;
    node->sel_try = 0;
    node->sel_win = 0;
    node->sim_try = 0;
    node->sim_win = 0;

    return node;
}


void
tree_node_print (TreeNode * tree_node)
{
    char * colour_str;
    switch (tree_node->colour) {
        case White: colour_str = "White"; break;
        case Red: colour_str = "Red"; break;
        case Golden: colour_str = "Golden"; break;
        case Purple: colour_str = "Purple"; break;
        case Black: colour_str = "Black"; break;
        default: colour_str = "COLOUR_MISSING";
    }
    g_printf ("res_code: %d\ncolour: %s\ninput: %s\nscore: %lf\n\n",
              tree_node->response_code, colour_str, tree_node->input_prefix, tree_node->score);
}


void
tree_node_free (TreeNode * tree_node)
{
    g_free (tree_node->input_prefix); /* causes segfault*/
    g_free (tree_node);
}


gboolean
traverse_free_func (GNode * gnode, gpointer data)
{
    tree_node_free (TreeNode(gnode->data));
    return FALSE;
}





/* EOF */
