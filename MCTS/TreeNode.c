#include <glib.h>
#include <glib/gprintf.h>
#include <float.h>
#include <math.h>

/*
 * Defines a macro for casting an object into a 'TreeNode'
 */
#define TreeNode(o) (TreeNode*)(o)

enum node_colour{White, Red, Golden, Purple, Black};

typedef struct
{
    // property
    gint response_code;
    enum node_colour colour;
    gboolean fully_explored;

    // input generation
    gchar * input_prefix;

    // statistics
    gdouble score;
    gint sel_try;
    gint sel_win;
    gint sim_try;
    gint sim_win;

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


int
main (int argc, char *argv[])
{
    GNode * root;
    GNode * child1;
    GNode * child2;

    root = g_node_new (tree_node_new(202, Red, "a"));
    child1 = g_node_new (tree_node_new(210, Red, "child1_str"));
    child2 = g_node_new (tree_node_new(300, Red, "child2_str"));
    g_node_append (root, child1);
    g_node_insert_after (root, child1, child2);
    /* Fetch data */
    GNode * first_child;
    GNode * sibling;
    first_child = g_node_first_child (root);
    sibling = g_node_next_sibling (first_child);
    tree_node_print (TreeNode(root->data));
    tree_node_print (TreeNode(first_child->data));
    tree_node_print (TreeNode(sibling->data));

    (TreeNode(child2->data))->colour = Red;

    tree_node_print (TreeNode(first_child->data));
    tree_node_print (TreeNode(sibling->data));
    tree_node_print (TreeNode(first_child->data));
    /* We must free everything now. */
    g_node_traverse
            (
                    root,
                    G_IN_ORDER,
                    G_TRAVERSE_ALL,
                    -1,
                    traverse_free_func,
                    NULL
            );
    g_node_destroy (root);
    return 0;
}

/* EOF */
