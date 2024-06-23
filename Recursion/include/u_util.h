#ifndef UTIL_H
#define UTIL_H

#include "u_errors.h"

#define ARRAY_SIZE(arr) (int32_t)(sizeof(arr) / sizeof(arr[0]))

enum e_key {
    FIRST_KEY   = 0,
    SECOND_KEY,
    MAX_KEY
};

enum e_dir {
    LEFT    = 0,
    MIDDLE,
    RIGHT
};

enum e_enable_flag {
    DISABLED    = 0,
    ENABLED
};

struct st_update_info
{
    int32_t             value_up;
    int32_t             value_split;
    e_enable_flag_t     update_flag;
};

struct st_tree_node
{
    int32_t             keys[MAX_KEY];
    st_update_info_t    update_info;
    st_tree_node_t      *p_left_child;
    st_tree_node_t      *p_middle_child;
    st_tree_node_t      *p_right_child;
    bool_t              is_root;
};

e_retcode_t insert(st_tree_node_t **const pp_root, const int32_t key);
st_tree_node_t *search(const st_tree_node_t *const p_start_node, const int32_t searched_key);
e_retcode_t delete(st_tree_node_t **const pp_root, const int32_t key);
void inorder_traverse(const st_tree_node_t *const p_tree_node);
void preorder_traverse(const st_tree_node_t *const p_tree_node);
void postorder_traverse(const st_tree_node_t *const p_tree_node);

#endif