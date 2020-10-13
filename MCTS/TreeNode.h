//
// Created by Dongge Liu on 4/10/20.
//

#ifndef AFLNET_TREENODE_H
#define AFLNET_TREENODE_H

/* libraries */
#include "glib_helper.h"
#include <float.h>
#include <math.h>
#include <unistd.h>
#include <assert.h>

#define SEED 0
#define DIGITS_IN_RESPONSE_CODE 12
#define RANDOM_NUMBER_GENERATOR g_rand_new_with_seed(SEED)

/* Defines a macro for casting an object into a 'TreeNode' */
#define TreeData(o) (TreeData*)(o)
#define TreeNode GNode

enum node_colour{White, Red, Golden, Purple, Black};
enum score_function{Random, UCT};

typedef struct
{
    // property
    int response_code;
    enum node_colour colour;
    gboolean fully_explored;
    gboolean exhausted;

    // input generation
    gchar * input_prefix;

    // statistics
    gdouble score;
    gdouble sel_try;
    gdouble sel_win;
    gdouble sim_try;
    gdouble sim_win;

}TreeNodeData;


/* Hyper-parameters */
gdouble RHO = 1.414;  //sqrt(2)
gint MIN_SAMPLES = 1;
gint MAX_SAMPLES = 100;
gint CONEX_TIMEOUT = 0;
gboolean PERSISTENT = FALSE;
gboolean COVERAGE_ONLY = TRUE;
enum score_function SCORE_FUNCTION = UCT;


/* Functions */
/* ============================================== TreeNode Functions ============================================== */
TreeNodeData * new_tree_node_data (int response_code, enum node_colour colour, const gchar * input_prefix);

TreeNode * new_tree_node(TreeNodeData * tree_data);

TreeNodeData * get_tree_node_data(TreeNode * tree_node);

double compute_exploitation_score(TreeNode * tree_node);

double compute_exploration_score(TreeNode * tree_node);

double tree_node_score(TreeNode * tree_node);

gboolean is_fully_explored(TreeNode *  tree_node);

gboolean is_leaf(TreeNode * tree_node);

gboolean fits_fish_bone_optimisation(TreeNode * tree_node);

TreeNode * best_child(TreeNode * tree_node);

char * mutate(TreeNode * tree_node);

TreeNode * exists_child(TreeNode *  tree_node, int target_response_code);

TreeNode * append_child(TreeNode * tree_node, int child_response_code, enum node_colour colour, char * input_prefix);

void print_reversed_path(TreeNode * tree_node);

int colour_encoder(enum node_colour colour);

void tree_node_print (TreeNode * tree_node);

/* ============================================== TreeNode Functions ============================================== */
/* ================================================ MCTS Functions ================================================ */

TreeNode * Selection(TreeNode * parent_tree_node);
char * Simulation(TreeNode * target);
gboolean Expansion(TreeNode * tree_node, int * response_codes, int len_codes, char ** input_prefix, TreeNode ** execution_leaf);
void Propagation(TreeNode * selection_leaf, TreeNode * execution_leaf, gboolean is_new);

/* ================================================ MCTS Functions ================================================ */

/* Precomputation */
//static TreeNode * ROOT = new_tree_node(new_tree_node_data(0,White,""));

#endif //AFLNET_TREENODE_H

