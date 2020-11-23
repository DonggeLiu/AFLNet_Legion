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

#include "../alloc-inl.h"
#include "../aflnet.h"
//#include "../afl-fuzz.c"


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
    // info
    gint id;
    u32 * path;
    u32 path_len;

    // statistics
    gint selected;
    gint discovered;

    // input generation
    void ** seeds;                  /* keeps all seeds reaching this node -- can be casted to struct seed_info_t* */
    int seeds_count;
    TreeNode * simulation_child;

    // property
    enum node_colour colour;
    gboolean fully_explored;
    gboolean exhausted;
}TreeNodeData;


typedef struct
{
    struct queue_entry * q;
    int selected;
    int discovered;
}seed_info_t;


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
TreeNodeData * new_tree_node_data (int response_code, enum node_colour colour);

TreeNode * new_tree_node(TreeNodeData * tree_data);

TreeNodeData * get_tree_node_data(TreeNode * tree_node);

TreeNode * get_simulation_child(TreeNode * tree_node);

double tree_node_exploitation_score(TreeNode * tree_node);

double tree_node_exploration_score(TreeNode * tree_node);

double tree_node_score(TreeNode * tree_node);

gboolean is_fully_explored(TreeNode *  tree_node);

gboolean is_leaf(TreeNode * tree_node);

gboolean fits_fish_bone_optimisation(TreeNode * tree_node);

TreeNode * best_child(TreeNode * tree_node);

char * mutate(TreeNode * tree_node);

TreeNode * exists_child(TreeNode *  tree_node, int target_response_code);

TreeNode * append_child(TreeNode * tree_node, int child_response_code, enum node_colour colour);

void print_reversed_path(TreeNode * tree_node);
u32 * collect_node_path(TreeNode * tree_node, u32 * path_len);
void print_path(TreeNode * tree_node);
u32 * collect_region_path(region_t region, u32 * path_len);

int colour_encoder(enum node_colour colour);

void tree_node_print (TreeNode * tree_node);

seed_info_t * construct_seed_with_queue_entry(struct queue_entry * q);
void add_seed_to_node(seed_info_t * seed, TreeNode * node);
void find_M2_region(seed_info_t * seed, TreeNode * tree_node, u32 * M2_start_region_ID, u32 * M2_region_count);
/* ============================================== TreeNode Functions ============================================== */
/* ================================================ MCTS Functions ================================================ */
TreeNode * select_tree_node(TreeNode * parent_tree_node);
seed_info_t * select_seed(TreeNode * tree_node_selected);
TreeNode * Initialisation();
seed_info_t * Selection(TreeNode * parent_tree_node);
char * Simulation(TreeNode * target);
TreeNode * Expansion(TreeNode * tree_node, seed_info_t * seed, int * response_codes, int len_codes, gboolean * is_new);
void Propagation(TreeNode * selection_leaf, TreeNode * execution_leaf, gboolean is_new);

/* ================================================ MCTS Functions ================================================ */

/* Precomputation */
//static TreeNode * ROOT = new_tree_node(new_tree_node_data(0,White,""));
extern TreeNode * ROOT;

#endif //AFLNET_TREENODE_H

