//
// Created by Dongge Liu on 4/10/20.
//

#ifndef MCTS_H
#define MCTS_H

/* libraries */
#include "glib_helper.h"
#include "logging.h"
//#include "utls.h"

#include <float.h>
#include <math.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "alloc-inl.h"
#include "aflnet.h"

#define _GNU_SOURCE

#define SEED 0
#define DIGITS_IN_RESPONSE_CODE 12
#define RANDOM_NUMBER_GENERATOR g_rand_new_with_seed(SEED)

/* Defines a macro for casting an object into a 'TreeNode' */
#define TreeData(o) (TreeData*)(o)
#define TreeNode GNode

//#define MAX_TREE_LOG_DEPTH 5

enum node_colour{White, Red, Golden, Purple, Black};
enum score_function{Random, UCT};

struct queue_entry {

    u8* fname;                          /* File name for the test case      */
    u32 len;                            /* Input length                     */

    u8  cal_failed,                     /* Calibration failed?              */
    trim_done,                      /* Trimmed?                         */
    was_fuzzed,                     /* Had any fuzzing done yet?        */
    passed_det,                     /* Deterministic stages passed?     */
    has_new_cov,                    /* Triggers new coverage?           */
    var_behavior,                   /* Variable behavior?               */
    favored,                        /* Currently favored?               */
    fs_redundant;                   /* Marked as redundant in the fs?   */

    u32 bitmap_size,                    /* Number of bits set in bitmap     */
    exec_cksum;                     /* Checksum of the execution trace  */

    u64 exec_us,                        /* Execution time (us)              */
    handicap,                       /* Number of queue cycles behind    */
    depth;                          /* Path depth                       */

    u8* trace_mini;                     /* Trace bytes, if kept             */
    u32 tc_ref;                         /* Trace bytes ref count            */

    struct queue_entry *next,           /* Next element, if any             */
    *next_100;       /* 100 elements ahead               */

    region_t *regions;                  /* Regions keeping information of message(s) sent to the server under test */
    u32 region_count;                   /* Total number of regions in this seed */
    u32 index;                          /* Index of this queue entry in the whole queue */
    u32 generating_state_id;            /* ID of the start at which the new seed was generated */
    u8 is_initial_seed;                 /* Is this an initial seed */
    u32 unique_state_count;             /* Unique number of states traversed by this queue entry */

};

typedef struct
{
    struct queue_entry* q; // Pointing to a specific queue entry/seed
    u32 parent_index;
    u32 selected;
    u32 discovered;
} seed_info_t;

typedef struct
{
    // info
    u32 id;
    u32 *path;
    u32 path_len;

    // statistics
    u32 selected;
    u32 discovered;

    // input generation
    seed_info_t **seeds; // keeps all seeds reaching this node -- can be casted to struct seed_info_t*
    u32 seeds_count;
    u32* region_indices; // Pointing to the index of the region corresponds to the node
    TreeNode *simulation_child;

    // property
    enum node_colour colour;
    gboolean fully_explored;
    gboolean exhausted;
} TreeNodeData;

/* Precomputation */
//static TreeNode * ROOT = new_tree_node(new_tree_node_data(0,White,""));
extern TreeNode * ROOT;


/* Core functions */
/* ============================================== TreeNode Functions ============================================== */
TreeNodeData* new_tree_node_data(u32 response_code, enum node_colour colour, u32* path, u32 path_len);

TreeNode* new_tree_node(TreeNodeData* tree_data);

TreeNodeData* get_tree_node_data(TreeNode* tree_node);

TreeNode* get_simulation_child(TreeNode* tree_node);

double tree_node_exploitation_score(TreeNode* tree_node);

double tree_node_exploration_score(TreeNode* tree_node);

double tree_node_score(TreeNode* tree_node);

gboolean is_fully_explored(TreeNode*  tree_node);

gboolean is_leaf(TreeNode* tree_node);

gboolean fits_fish_bone_optimisation(TreeNode* tree_node);

TreeNode* best_child(TreeNode* tree_node);

char* mutate(TreeNode* tree_node);

TreeNode* exists_child(TreeNode* tree_node, u32 target_response_code);

TreeNode* append_child(TreeNode* tree_node, u32 child_response_code, enum node_colour colour, u32* path, u32 path_len);

void tree_log(TreeNode* tree_node, TreeNode* mark_node, int indent, int found);
char* tree_node_repr(TreeNode* tree_node);
char* seed_repr(TreeNode* tree_node, uint seed_index, seed_info_t* seed);
/* ================================================ MCTS Functions ================================================ */
TreeNode* select_tree_node(TreeNode* parent_tree_node);
seed_info_t* select_seed(TreeNode* tree_node_selected);
TreeNode* Initialisation(uint log_lvl, uint tree_dp, uint ign_ast, double rho);
seed_info_t* Selection(TreeNode** parent_tree_node);
char* Simulation(TreeNode* target);
TreeNode* Expansion(TreeNode* tree_node, struct queue_entry* q, u32* response_codes, u32 len_codes, gboolean* is_new);
void Propagation(TreeNode* leaf_selected, seed_info_t* seed_selected, gboolean is_new);


/* Helper functions */
void print_reversed_path(TreeNode* tree_node);

u32* collect_node_path(TreeNode* tree_node, u32* path_len);

void print_path(TreeNode* tree_node);

u32* collect_region_path(region_t region, u32* path_len);

int colour_encoder(enum node_colour colour);

void preprocess_queue_entry(struct queue_entry* q);
//void tree_node_print (TreeNode* tree_node);

seed_info_t* construct_seed_with_queue_entry(struct queue_entry* q);

void add_seed_to_node(seed_info_t* seed, u32 matching_region_index, TreeNode* node);

char* node_path_str(TreeNode* tree_node);

#endif //MCTS_H

