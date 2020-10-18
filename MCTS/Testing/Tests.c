//
// Created by Dongge Liu on 4/10/20.
//

#include "Tests.h"

//TreeNode * initialise_node(TreeNode * parent, int response_code, int selected, int new)
//{
//    TreeNode * child = append_child(parent, response_code, White);
//
//}


int
main (int argc, char *argv[])
{
    TreeNode * child0, * child1, * child2, * child3,
    * grandchild01, * grandchild02, * grandchild03,
    * grandchild11, * grandchild12, * grandchild20;
    u32 a = 3, b = 9;
    g_printf("%u / %u = %lf", a, b, (double) a / b);

    g_printf ("GLib version: %d.%d.%d\n\n", glib_major_version,glib_minor_version,glib_micro_version);

    TreeNode * ROOT = Initialisation();
    get_tree_node_data(ROOT)->stats.paths_discovered = 3;
    get_tree_node_data(ROOT)->stats.selected_times = 9;
//    g_print("ROOT\n");
//    g_printf("%u\n", get_tree_node_data(ROOT)->stats.paths_discovered);
//    g_printf("%u\n", get_tree_node_data(ROOT)->stats.selected_times);
//    g_printf("%lf\n", tree_node_exploitation_score(ROOT));
//    g_printf("%lf\n", tree_node_exploration_score(ROOT));


    child0 = append_child(ROOT,400, White);
    get_tree_node_data(child0)->stats.paths_discovered = 1;
    get_tree_node_data(child0)->stats.selected_times = 2;
    g_print("Child0\n");
    tree_node_print(child0);
    g_printf("Path Discovered: %u\n", get_tree_node_data(child0)->stats.paths_discovered);
    g_printf("Selected Times: %u\n", get_tree_node_data(child0)->stats.selected_times);
    g_printf("Exploitation score: %lf\n", tree_node_exploitation_score(child0));
    g_printf("Exploration score: %lf\n\n", tree_node_exploration_score(child0));

    child1 = append_child(ROOT,100, White);
    get_tree_node_data(child1)->stats.paths_discovered = 1;
    get_tree_node_data(child1)->stats.selected_times = 2;

//    g_print("child1\n");
//    g_printf("%u\n", get_tree_node_data(child1)->stats.paths_discovered);
//    g_printf("%u\n", get_tree_node_data(child1)->stats.selected_times);
//    g_printf("%lf\n", tree_node_exploitation_score(child1));
//    g_printf("%lf\n", tree_node_exploration_score(child1));

    child2 = append_child(ROOT,200, White);
    get_tree_node_data(child2)->stats.paths_discovered = 1;
    get_tree_node_data(child2)->stats.selected_times = 2;
    get_tree_node_data(get_tree_node_data(child2)->simulation_child)->stats.paths_discovered = 1;
    get_tree_node_data(get_tree_node_data(child2)->simulation_child)->stats.selected_times = 2;
//    g_print("child2\n");
//    g_printf("%u\n", get_tree_node_data(child2)->stats.paths_discovered);
//    g_printf("%u\n", get_tree_node_data(child2)->stats.selected_times);
//    g_printf("%lf\n", tree_node_exploitation_score(child2));
//    g_printf("%lf\n", tree_node_exploration_score(child2));

    child3 = append_child(ROOT, 300, White);
    get_tree_node_data(child3)->stats.paths_discovered = 1;
    get_tree_node_data(child3)->stats.selected_times = 3;
//    g_print("child3\n");
//    g_printf("%u\n", get_tree_node_data(child3)->stats.paths_discovered);
//    g_printf("%u\n", get_tree_node_data(child3)->stats.selected_times);
//    g_printf("%lf\n", tree_node_exploitation_score(child3));
//    g_printf("%lf\n", tree_node_exploration_score(child3));

    /* TOASK:
     *  1s. using id 410 works, but changing it to 010 will not print out 8 instead
     *      Similarly, 020 becomes 16
     *      How does it become octal?
     */
    grandchild01 = append_child(child0, 410, White);
    get_tree_node_data(grandchild01)->stats.paths_discovered = 2;
    get_tree_node_data(grandchild01)->stats.selected_times = 3;
    g_print("grandchild01\n");
    tree_node_print(grandchild01);
    g_printf("Path Discovered: %u\n", get_tree_node_data(child3)->stats.paths_discovered);
    g_printf("Selected Times: %u\n", get_tree_node_data(child3)->stats.selected_times);
    g_printf("Exploitation score: %lf\n", tree_node_exploitation_score(child3));
    g_printf("Exploration score: %lf\n\n", tree_node_exploration_score(child3));

    grandchild02 = append_child(child0, 020, White);
    get_tree_node_data(grandchild02)->stats.paths_discovered = 2;
    get_tree_node_data(grandchild02)->stats.selected_times = 4;

    grandchild03 = append_child(child0, 030, White);
    get_tree_node_data(grandchild03)->stats.paths_discovered = 3;
    get_tree_node_data(grandchild03)->stats.selected_times = 4;

    grandchild11 = append_child(child1, 110, White);
    get_tree_node_data(grandchild11)->stats.paths_discovered = 2;
    get_tree_node_data(grandchild11)->stats.selected_times = 3;

    grandchild12 = append_child(child1, 120, White);
    get_tree_node_data(grandchild12)->stats.paths_discovered = 2;
    get_tree_node_data(grandchild12)->stats.selected_times = 4;

    grandchild20 = append_child(child2, 220, White);
    get_tree_node_data(grandchild20)->stats.paths_discovered = 2;
    get_tree_node_data(grandchild20)->stats.selected_times = 2;

    tree_print(ROOT, NULL, 0,0);

    print_path(grandchild01);

    Selection(ROOT);

//    grandchild1 = new_tree_node(new_tree_node_data(211, White));
//    child2 = new_tree_node(new_tree_node_data(202, Red));
//    tree_node_print (ROOT);
//    g_node_append (ROOT, child1);
//    g_node_append (ROOT, child2);
//    g_node_append (child1, grandchild1);
//    tree_node_print(g_node_first_child(ROOT));
//    tree_node_print(ROOT);
//    print_reversed_path(child2);
////    print_path(child2);
//    gboolean is_new = FALSE;
//    tree_print(ROOT, child1, 0,0);
//    TreeNode * leaf = malloc(sizeof(TreeNode));
//    leaf = Expansion(ROOT, (int[]){201, 1, 2, 3}, 4, &is_new);
    //    print_path(leaf);

//    TreeNode ** parent2 = malloc(8);
////    tree_node_print(*parent2);
//    parent(child2, parent2);
//    tree_node_print(*parent2);

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