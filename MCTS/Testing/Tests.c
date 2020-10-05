//
// Created by Dongge Liu on 4/10/20.
//

#include "Tests.h"

int
main (int argc, char *argv[])
{
    TreeNode * root, * child1, * child2, * grandchild1;

    g_printf ("GLib version: %d.%d.%d\n\n", glib_major_version,glib_minor_version,glib_micro_version);

    root = new_tree_node(new_tree_node_data("200", Red, ""));
    child1 = new_tree_node(new_tree_node_data("201", White, "1"));
    grandchild1 = new_tree_node(new_tree_node_data("211", White, "11"));
    child2 = new_tree_node(new_tree_node_data("202", Red, "2"));
    tree_node_print (root);
    g_node_append (root, child1);
    g_node_append (root, child2);
    g_node_append (child1, grandchild1);
    tree_node_print(g_node_first_child(root));
    tree_node_print(root);
    print_reversed_path(child2);

    tree_print(root, child1, 0,0);

//    tree_node_print(match_child(ROOT, 201));

//    tree_node_print (ROOT);
//    print_reversed_path(child1);
//    tree_node_print (child1);
//    root = g_node_new (tree_node_new(202, Red, "a"));
//    child1 = g_node_new (tree_node_new(210, Red, "child1_str"));
//    child2 = g_node_new (tree_node_new(300, Red, "child2_str"));
//    g_node_append (root, child1);
//    g_node_insert_after (root, child1, child2);
//    /* Fetch data */
//    GNode * first_child;
//    GNode * sibling;
//    first_child = g_node_first_child (root);
//    sibling = g_node_next_sibling (first_child);
//    tree_node_print (TreeNode(root->data));
//    tree_node_print (TreeNode(first_child->data));
//    tree_node_print (TreeNode(sibling->data));
//
//    (TreeNode(child2->data))->colour = Red;
//
//    tree_node_print (TreeNode(first_child->data));
//    tree_node_print (TreeNode(sibling->data));
//    tree_node_print (TreeNode(first_child->data));
//    /* We must free everything now. */
//    g_node_traverse
//            (
//                    root,
//                    G_IN_ORDER,
//                    G_TRAVERSE_ALL,
//                    -1,
//                    traverse_free_func,
//                    NULL
//            );
//    g_node_destroy (root);
    return 0;
}