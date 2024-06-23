#include "u_stack_ctrl.h"
#include "u_util.h"

static st_tree_node_t *create_node(const int32_t key, const bool_t is_root);
static bool_t node_is_full(const st_tree_node_t *const p_tree_node);
static bool_t node_is_vacant(const st_tree_node_t *const p_tree_node);
static bool_t node_is_leaf(const st_tree_node_t *const p_tree_node);
static inline bool_t key_on_the_left(const int32_t key, const st_tree_node_t *const p_tree_node);
static inline bool_t key_in_the_middle(const int32_t key, const st_tree_node_t *const p_tree_node);
static inline bool_t key_on_the_right(const int32_t key, const st_tree_node_t *const p_tree_node);
static void split(st_tree_node_t **const pp_root, st_tree_node_t *const p_current, const e_dir_t dir);
static void merge(st_tree_node_t **pp_root, st_tree_node_t *p_parent, e_dir_t target_dir, e_dir_t merged_dir, st_tree_node_t *p_target_delete);
static void insert_to_tree(st_tree_node_t **const pp_root, st_tree_node_t *const candidate, const int32_t key, e_dir_t dir);
static void delete_from_node(st_tree_node_t **const pp_root, st_tree_node_t *const p_current, const int32_t key);
static void delete_key(st_tree_node_t *const p_tree_node, const e_key_t position);
static void node_shift(st_tree_node_t *const p_tree_node, e_dir_t dir);
static void delete_one_key_leaf_node(st_tree_node_t **pp_root, st_tree_node_t *p_current);
static st_tree_node_t *get_leftmost(st_tree_node_t *p_tree_node);
static st_tree_node_t *inorder_successor(st_tree_node_t *p_tree_node, const e_key_t key_position);
static void process_inorder_successor(st_tree_node_t **pp_root, st_tree_node_t *p_current, const e_key_t key_position);
static void delete_internal_node(st_tree_node_t **pp_root, st_tree_node_t *p_current, int32_t key_to_delete);

static st_stack_t stack           = { NULL };
static st_stack_t stack_for_split = { NULL };
static e_enable_flag_t delete_flag = DISABLED;
static bool_t root_change = false;

static st_tree_node_t *create_node(const int32_t key, const bool_t is_root)
{
    st_tree_node_t *node;

    node = (st_tree_node_t *)malloc(sizeof(st_tree_node_t));
    if (NULL != node)
    {
        node->keys[FIRST_KEY]           = key;
        node->keys[SECOND_KEY]          = (-1);   /**< -1 means blank space */
        node->p_left_child              = NULL;
        node->p_middle_child            = NULL;
        node->p_right_child             = NULL;
        node->is_root                   = is_root;
        node->update_info.update_flag   = DISABLED;
        node->update_info.value_up      = 0;
        node->update_info.value_split   = 0;
    }

    return node;
}

static bool_t node_is_full(const st_tree_node_t *const p_tree_node)
{
    return (((-1) != p_tree_node->keys[FIRST_KEY]) && ((-1) != p_tree_node->keys[SECOND_KEY]));
}

static bool_t node_is_vacant(const st_tree_node_t *const p_tree_node)
{
    return (((-1) == p_tree_node->keys[FIRST_KEY]) && ((-1) == p_tree_node->keys[SECOND_KEY]));
}

static bool_t node_is_leaf(const st_tree_node_t *const p_tree_node)
{
    bool_t checked = false;

    if (true == node_is_full(p_tree_node))
    {
        checked = ((NULL == p_tree_node->p_left_child) && (NULL == p_tree_node->p_middle_child) && (NULL == p_tree_node->p_right_child));
    }
    else
    {
        checked = ((NULL == p_tree_node->p_left_child) && (NULL == p_tree_node->p_right_child));
    }

    return checked;
}

static inline bool_t key_on_the_left(const int32_t key, const st_tree_node_t *const p_tree_node)
{
    return (key < p_tree_node->keys[FIRST_KEY]);
}

static inline bool_t key_in_the_middle(const int32_t key, const st_tree_node_t *const p_tree_node)
{
    return ((key > p_tree_node->keys[FIRST_KEY]) && (key < p_tree_node->keys[SECOND_KEY]));
}

static inline bool_t key_on_the_right(const int32_t key, const st_tree_node_t *const p_tree_node)
{
    return (key > p_tree_node->keys[SECOND_KEY]);
}

static void split(st_tree_node_t **const pp_root, st_tree_node_t *const p_current, const e_dir_t dir)
{
    st_tree_node_t  *new_root = NULL;
    st_tree_node_t  *split_node = NULL;
    st_tree_node_t  *parent = NULL;
    st_tree_node_t  *popped_node = NULL;

    if (ENABLED == p_current->update_info.update_flag)
    {
        /* If p_current is the root */
        if (true == p_current->is_root)
        {
            /* Node is always full in this case as it has to be split up. So there's no need to check whether it's full or not */
            /* p_current is no longer the root */
            p_current->is_root = false;

            /* Create new root and new split_node */
            new_root   = create_node(p_current->update_info.value_up, true);
            split_node = create_node(p_current->update_info.value_split, false);

            /* Finish splitting and promoting */
            p_current->update_info.update_flag = DISABLED;

            /* Determine the side that the node in the stack_for_split is on */
            popped_node = stack_pop(&stack_for_split);

            if (LEFT == dir)
            {
                /* Assign split_node */
                split_node->p_left_child   = p_current->p_middle_child;
                split_node->p_middle_child = p_current->p_right_child;
            }
            else if (MIDDLE == dir)
            {
                /* Assign split_node */
                split_node->p_left_child   = popped_node;
                split_node->p_middle_child = p_current->p_right_child;
            }
            else
            {
                /* Assign split_node */
                split_node->p_left_child   = p_current->p_right_child;
                split_node->p_middle_child = popped_node;
            }

            /* Remove 2nd key in p_current */
            p_current->keys[SECOND_KEY] = (-1);

            /* Assign new root */
            new_root->p_left_child   = p_current;
            new_root->p_middle_child = split_node;

            /* Promote new root */
            *pp_root = new_root;
        }
        else
        {
            split_node = create_node(p_current->update_info.value_split, false);
            p_current->update_info.update_flag = DISABLED;

            /* Popped from stack */
            parent = stack_pop(&stack);

            if (NULL != parent)
            {
                /* Parent node is full */
                if (true == node_is_full(parent))
                {
                    /* Push the split_node to the stack */
                    stack_push(&stack_for_split, (uintptr_t)split_node);

                    /* Enable splitting and promoting */
                    parent->update_info.update_flag = ENABLED;

                    if (true == key_on_the_left(p_current->update_info.value_up, parent))
                    {
                        /* Prepare for splitting and promoting */
                        parent->update_info.value_up    = parent->keys[FIRST_KEY];
                        parent->update_info.value_split = parent->keys[SECOND_KEY];

                        /* Move up the key from p_current to parent */
                        parent->keys[FIRST_KEY] = p_current->update_info.value_up;

                        /* Split up the parent */
                        split(pp_root, parent, LEFT);

                        /* Pop the stack to assign nodes if needed */
                        popped_node = stack_pop(&stack_for_split);

                        /* Assign parent */
                        parent->p_left_child   = p_current;
                        parent->p_middle_child = split_node;
                        parent->p_right_child  = NULL;
                    }
                    else if (true == key_in_the_middle(p_current->update_info.value_up, parent))
                    {
                        /* Prepare for splitting and promoting */
                        parent->update_info.value_up    = p_current->update_info.value_up;
                        parent->update_info.value_split = parent->keys[SECOND_KEY];

                        /* Split up the parent */
                        split(pp_root, parent, MIDDLE);

                        /* Pop the stack to assign nodes if needed */
                        popped_node = stack_pop(&stack_for_split);

                        /* Assign parent */
                        parent->p_middle_child = p_current;
                        parent->p_right_child  = NULL;
                    }
                    else
                    {
                        /* Prepare for splitting and promoting */
                        parent->update_info.value_up    = parent->keys[SECOND_KEY];
                        parent->update_info.value_split = p_current->update_info.value_up;

                        /* Split up the parent */
                        split(pp_root, parent, RIGHT);

                        /* Pop the stack to assign nodes if needed */
                        popped_node = stack_pop(&stack_for_split);

                        /* Assign parent */
                        parent->p_right_child  = NULL;
                    }

                    /* Assign split_node if needed */
                    if (false == node_is_leaf(p_current))
                    {
                        if (popped_node == split_node)
                        {
                            /* Get the child */
                            popped_node = stack_pop(&stack_for_split);
                        }

                        if (LEFT == dir)
                        {
                            split_node->p_left_child   = p_current->p_middle_child;
                            split_node->p_middle_child = p_current->p_right_child;
                        }
                        else if (MIDDLE == dir)
                        {
                            split_node->p_left_child   = popped_node;
                            split_node->p_middle_child = p_current->p_right_child;
                        }
                        else
                        {
                            split_node->p_left_child   = p_current->p_right_child;
                            split_node->p_middle_child = popped_node;
                        }
                    }
                }
                /* Parent node has only one key */
                else
                {
                    if (true == key_on_the_left(p_current->update_info.value_up, parent))
                    {
                        parent->keys[SECOND_KEY] = parent->keys[FIRST_KEY];
                        parent->keys[FIRST_KEY]  = p_current->update_info.value_up;

                        /* Assign parent */
                        parent->p_right_child  = parent->p_middle_child;
                        parent->p_middle_child = split_node;
                    }
                    else
                    {
                        parent->keys[SECOND_KEY] = p_current->update_info.value_up;

                        /* Assign parent's middle */
                        parent->p_right_child  = split_node;
                    }

                    /* Assign split_node */
                    if (false == node_is_leaf(p_current))
                    {
                        split_node->p_middle_child = NULL;

                        if (LEFT == dir)
                        {
                            split_node->p_left_child   = p_current->p_middle_child;
                            split_node->p_middle_child = p_current->p_right_child;
                        }
                        else if (MIDDLE == dir)
                        {
                            popped_node = stack_pop(&stack_for_split);

                            if (NULL != popped_node)
                            {
                                split_node->p_left_child   = popped_node;
                                split_node->p_middle_child = p_current->p_right_child;
                            }
                        }
                        else
                        {
                            popped_node = stack_pop(&stack_for_split);

                            if (NULL != popped_node)
                            {
                                split_node->p_left_child   = p_current->p_right_child;
                                split_node->p_middle_child = popped_node;
                            }
                        }
                    }
                }

                /* Adjust p_current */
                p_current->keys[SECOND_KEY] = (-1);

                /* Finish splitting and promoting */
                p_current->update_info.update_flag = DISABLED;

                /* Clear stack */
                if (DISABLED == parent->update_info.update_flag)
                {
                    while (false == stack_is_empty(&stack))
                    {
                        (void)stack_pop(&stack);
                    }
                }
            }
        }
    }
}

static void insert_to_tree(st_tree_node_t **const pp_root, st_tree_node_t *const candidate, const int32_t key, e_dir_t dir)
{
    if (NULL == candidate)
    {
        *pp_root = create_node(key, true);
    }
    else
    {
        /* When candidate is a leaf */
        if (true == node_is_leaf(candidate))
        {
            if (true == node_is_full(candidate))
            {
                /* Enable splitting and promoting */
                candidate->update_info.update_flag = ENABLED;

                if (true == key_on_the_left(key, candidate))
                {
                    /* Prepare for splitting and promoting */
                    candidate->update_info.value_up    = candidate->keys[FIRST_KEY];
                    candidate->update_info.value_split = candidate->keys[SECOND_KEY];

                    /* Assign key to the candidate */
                    candidate->keys[FIRST_KEY] = key;
                }
                else if (true == key_in_the_middle(key, candidate))
                {
                    /* Prepare for splitting and promoting */
                    candidate->update_info.value_up    = key;
                    candidate->update_info.value_split = candidate->keys[SECOND_KEY];
                }
                else
                {
                    /* Prepare for splitting and promoting */
                    candidate->update_info.value_up    = candidate->keys[SECOND_KEY];
                    candidate->update_info.value_split = key;
                }
            }
            else
            {
                if (true == key_on_the_left(key, candidate))
                {
                    candidate->keys[SECOND_KEY] = candidate->keys[FIRST_KEY];
                    candidate->keys[FIRST_KEY]  = key;
                }
                else
                {
                    candidate->keys[SECOND_KEY] = key;
                }

                /* Disable splitting and promoting */
                candidate->update_info.update_flag = DISABLED;

                while (false == stack_is_empty(&stack))
                {
                    (void)stack_pop(&stack);
                }
            }
        }
        else
        {
            stack_push(&stack, (uintptr_t)candidate);

            if (true == node_is_full(candidate))
            {
                if (true == key_on_the_left(key, candidate))
                {
                    dir = LEFT;
                    insert_to_tree(pp_root, candidate->p_left_child, key, dir);
                }
                else if (true == key_in_the_middle(key, candidate))
                {
                    dir = MIDDLE;
                    insert_to_tree(pp_root, candidate->p_middle_child, key, dir);
                }
                else
                {
                    dir = RIGHT;
                    insert_to_tree(pp_root, candidate->p_right_child, key, dir);
                }
            }
            else
            {
                if (true == key_on_the_left(key, candidate))
                {
                    dir = LEFT;
                    insert_to_tree(pp_root, candidate->p_left_child, key, dir);
                }
                else
                {
                    dir = MIDDLE;
                    insert_to_tree(pp_root, candidate->p_middle_child, key, dir);
                }
            }
        }

        /* Self-balance */
        split(pp_root, candidate, dir);
    }
}

static void node_shift(st_tree_node_t *const p_tree_node, e_dir_t dir)
{
    if (LEFT == dir)
    {
        p_tree_node->keys[FIRST_KEY]  = p_tree_node->keys[SECOND_KEY];
        p_tree_node->keys[SECOND_KEY] = (-1);
    }
    else if (RIGHT == dir)
    {
        p_tree_node->keys[SECOND_KEY] = p_tree_node->keys[FIRST_KEY];
        p_tree_node->keys[FIRST_KEY]  = (-1);
    }
    else
    {
        /* Do nothing */
    }
}

static void merge(st_tree_node_t **pp_root, st_tree_node_t *p_parent, e_dir_t target_dir, e_dir_t merged_dir, st_tree_node_t *p_target_delete)
{
    st_tree_node_t *p_target_node;

    if (NULL == p_parent)
    {
        return;
    }

    if ((LEFT == target_dir) && (MIDDLE == merged_dir))
    {
        if ((true == node_is_full(p_parent->p_left_child)) || (true == node_is_full(p_parent->p_middle_child)))
        {
            return;
        }
    }

    if ((MIDDLE == target_dir) && (RIGHT == merged_dir))
    {
        if ((true == node_is_full(p_parent->p_middle_child)) || (true == node_is_full(p_parent->p_right_child)))
        {
            return;
        }
    }

    /* When parent is full (contains 2 keys) (has 3 children) */
    if (true == node_is_full(p_parent))
    {
        /* Merge middle sibling to left sibling */
        if ((LEFT == target_dir) && (MIDDLE == merged_dir))
        {
            /* When target_delete is the left child */
            if (p_target_delete == p_parent->p_left_child)
            {
                /* Merge 2 nodes */
                p_parent->p_left_child->keys[FIRST_KEY]  = p_parent->keys[FIRST_KEY];
                p_parent->p_left_child->keys[SECOND_KEY] = p_parent->p_middle_child->keys[FIRST_KEY];
                delete_key(p_parent, FIRST_KEY);

                /* Assign left child's children */
                /* When middle child of the left child is missing as left child is a non-leaf node */
                if (false == node_is_leaf(p_parent->p_left_child))
                {
                    if (NULL == p_parent->p_left_child->p_middle_child)
                    {
                        p_parent->p_left_child->p_middle_child = p_parent->p_middle_child->p_left_child;
                        p_parent->p_left_child->p_right_child  = p_parent->p_middle_child->p_middle_child;
                    }
                    else
                    {
                        /* Do nothing */
                    }
                }
                /* When left child is a leaf node */
                else
                {
                    /* Do nothing */
                }

                /* Destroy the middle node */
                free(p_parent->p_middle_child);

                /* Arrange parent's children */
                p_parent->p_middle_child = p_parent->p_right_child;
                p_parent->p_right_child  = NULL;
            }
            /* When target_delete is the middle child */
            else
            {
                /* Merge 2 nodes */
                p_parent->p_left_child->keys[SECOND_KEY] = p_parent->keys[FIRST_KEY];
                delete_key(p_parent, FIRST_KEY);

                /* When parent's children are leaf nodes */
                if (true == node_is_leaf(p_parent->p_left_child))
                {
                    /* Destroy the middle child */
                    free(p_parent->p_middle_child);

                    /* Adjust parent's children */
                    p_parent->p_middle_child = p_parent->p_right_child;
                    p_parent->p_right_child  = NULL;
                }
                /* When parent's children are non leaf nodes */
                else
                {
                    /* Do nothing *//* Confirmed! */
                }                
            }
        }
        /* Merge right sibling to middle sibling */
        else if ((MIDDLE == target_dir) && (RIGHT == merged_dir))
        {
            p_target_node = p_parent->p_middle_child;

            /* When target_delete is the middle child */
            if (p_target_delete == p_parent->p_middle_child)
            {
                /* Change target_delete to right child (merged node) */
                p_target_delete = p_parent->p_right_child;

                /* Merge 2 nodes */
                p_target_node->keys[FIRST_KEY]  = p_parent->keys[SECOND_KEY];
                p_target_node->keys[SECOND_KEY] = p_target_delete->keys[FIRST_KEY];
                delete_key(p_parent, SECOND_KEY);
                delete_key(p_target_delete, FIRST_KEY);

                /* Adjust children: Shift from right sibling */
                p_target_node->p_middle_child = p_target_delete->p_left_child;
                p_target_node->p_right_child  = p_target_delete->p_middle_child;
            }
            /* When target_delete is the right child */
            else
            {
                /* Merge 2 nodes */
                p_target_node->keys[SECOND_KEY] = p_parent->keys[SECOND_KEY];
                delete_key(p_parent, SECOND_KEY);

                /* Adjust children: Shift from right sibling */
                p_target_node->p_right_child = p_target_delete->p_left_child;
            }

            /* Destroy p_target_delete */
            free(p_target_delete);
            p_target_delete         = NULL;
            p_parent->p_right_child = NULL;
        }
        else
        {
            /* Do nothing (confirmed) */
        }
    }
    /* When parent has only one key (has 2 children) */
    else
    {
        /* When target delete is middle child */
        if (p_target_delete == p_parent->p_middle_child)
        {
            /* Move parent's key to the left child */
            p_parent->p_left_child->keys[SECOND_KEY] = p_parent->keys[FIRST_KEY];
            delete_key(p_parent, FIRST_KEY);

            /* In case children are non-leaf nodes */
            if (false == node_is_leaf(p_parent->p_left_child))
            {
                /* When 2 children of target_delete is not yet merged */
                if (NULL != p_target_delete->p_middle_child)
                {
                    merge(pp_root, p_target_delete, LEFT, MIDDLE, p_target_delete->p_middle_child);
                }

                p_parent->p_left_child->p_right_child = p_target_delete->p_left_child;
                p_target_delete->p_left_child         = NULL;
            }
            /* In case children are leaf nodes */
            else
            {
                /* Do nothing */
            }

            /* Destroy the middle node */
            free(p_target_delete);
            p_parent->p_middle_child = NULL;
        }
        /* When target delete is left child */
        else
        {
            /* Change target_delete to middle child because we merge into left child */
            p_target_delete = p_parent->p_middle_child;

            /* Merge middle sibling to left sibling */
            p_parent->p_left_child->keys[FIRST_KEY]  = p_parent->keys[FIRST_KEY];
            p_parent->p_left_child->keys[SECOND_KEY] = p_target_delete->keys[FIRST_KEY];
            delete_key(p_parent, FIRST_KEY);

            /* Assign children of the left child */
            if (NULL != p_parent->p_left_child->p_middle_child)
            {
                merge(pp_root, p_parent->p_left_child, LEFT, MIDDLE, p_parent->p_left_child->p_middle_child);
            }

            /* Adjust children: Shift from middle sibling */
            p_parent->p_left_child->p_middle_child = p_target_delete->p_left_child;
            p_parent->p_left_child->p_right_child  = p_target_delete->p_middle_child;

            /* Destroy the middle node */
            free(p_target_delete);
            p_parent->p_middle_child = NULL;
        }
    }

    /* Update root if needed */
    if ((true == p_parent->is_root) && (true == node_is_vacant(p_parent)))
    {
        p_parent->p_left_child->is_root = true;
        *pp_root = p_parent->p_left_child;
        free(p_parent);
        root_change = true;
    }
}

static void delete_key(st_tree_node_t *const p_tree_node, const e_key_t position)
{
    if (FIRST_KEY == position)
    {
        node_shift(p_tree_node, LEFT);
    }
    else
    {
        p_tree_node->keys[SECOND_KEY] = (-1);
    }
}

static void delete_one_key_leaf_node(st_tree_node_t **pp_root, st_tree_node_t *p_current)
{
    st_tree_node_t  *p_parent;

    /* Delete the key */
    delete_key(p_current, FIRST_KEY);

    p_parent = stack_pop(&stack);

    /* When current is not a root */
    if (NULL != p_parent)
    {
        /* When parent is full (contains 2 keys) */
        if (true == node_is_full(p_parent))
        {
            /* When current is the left child */
            if (p_current == p_parent->p_left_child)
            {
                /* When middle sibling is full (contains 2 keys) */
                if (true == node_is_full(p_parent->p_middle_child))
                {
                    /* Borrow from middle child */
                    p_current->keys[FIRST_KEY] = p_parent->keys[FIRST_KEY];
                    p_parent->keys[FIRST_KEY]  = p_parent->p_middle_child->keys[FIRST_KEY];

                    /* Delete borrowed key from borrowed node */
                    delete_from_node(pp_root, p_parent->p_middle_child, p_parent->p_middle_child->keys[FIRST_KEY]);
                }
                /* When middle sibling has only one key */
                else
                {
                    /* Merge with the middle */
                    merge(pp_root, p_parent, LEFT, MIDDLE, p_current);
                }
            }
            /* When current is the middle child */
            else if (p_current == p_parent->p_middle_child)
            {
                /* When the right child is full (contains 2 keys) */
                if (true == node_is_full(p_parent->p_right_child))
                {
                    /* Borrow from the right sibling */
                    p_current->keys[FIRST_KEY] = p_parent->keys[SECOND_KEY];
                    p_parent->keys[SECOND_KEY] = p_parent->p_right_child->keys[FIRST_KEY];

                    /* Delete borrowed key in the borrowed node (right sibling) */
                    delete_from_node(pp_root, p_parent->p_right_child, p_parent->p_right_child->keys[FIRST_KEY]);
                }
                /* When the left child is full (contains 2 keys) */
                else if (true == node_is_full(p_parent->p_left_child))
                {
                    /* Borrow from the left sibling */
                    node_shift(p_current, RIGHT);
                    p_current->keys[FIRST_KEY] = p_parent->keys[FIRST_KEY];
                    p_parent->keys[FIRST_KEY]  = p_parent->p_left_child->keys[SECOND_KEY];

                    /* Delete borrowed key in the borrowed node (left sibling) */
                    delete_from_node(pp_root, p_parent->p_left_child, p_parent->p_left_child->keys[SECOND_KEY]);
                }
                /* When the left sibling and the right sibling both have only one key */
                else
                {
                    /* Merge with the right sibling */
                    merge(pp_root, p_parent, MIDDLE, RIGHT, p_current);
                }
            }
            /* When current is the right child */
            else
            {
                /* When middle sibling is full (contains 2 keys) */
                if (true == node_is_full(p_parent->p_middle_child))
                {
                    /* Borrow from middle child */
                    p_current->keys[FIRST_KEY] = p_parent->keys[SECOND_KEY];
                    p_parent->keys[SECOND_KEY] = p_parent->p_middle_child->keys[SECOND_KEY];

                    /* Delete borrowed key from borrowed node */
                    delete_key(p_parent->p_middle_child, SECOND_KEY);
                }
                /* When middle sibling has only one key */
                else
                {
                    merge(pp_root, p_parent, MIDDLE, RIGHT, p_current);
                }
            }
        }
        /* When parent has only one key */
        else
        {
            /* When current is the left child */
            if (p_current == p_parent->p_left_child)
            {
                /* When middle sibling is full (contains 2 keys) */
                if (true == node_is_full(p_parent->p_middle_child))
                {
                    /* Borrow key from middle sibling */
                    p_current->keys[FIRST_KEY] = p_parent->keys[FIRST_KEY];
                    p_parent->keys[FIRST_KEY]  = p_parent->p_middle_child->keys[FIRST_KEY];

                    /* Delete borrowed key in the borrowed node (middle sibling) */
                    delete_from_node(pp_root, p_parent->p_middle_child, p_parent->p_middle_child->keys[FIRST_KEY]);
                }
                /* When middle sibling has only one key */
                else
                {
                    if (ENABLED == delete_flag)
                    {
                        merge(pp_root, p_parent, LEFT, MIDDLE, p_current);

                        delete_flag = DISABLED;
                    }

                    /* Delete key in parent */
                    if (false == root_change)
                    {
                        delete_from_node(pp_root, p_parent, p_parent->keys[FIRST_KEY]);
                    }
                    else
                    {
                        root_change = false;
                    }
                }
            }
            /* When current is the middle child */
            else
            {
                /* When left sibling is full (contains 2 keys) */
                if (true == node_is_full(p_parent->p_left_child))
                {
                    /* Borrow a key from left sibling */
                    p_current->keys[FIRST_KEY] = p_parent->keys[FIRST_KEY];
                    p_parent->keys[FIRST_KEY]  = p_parent->p_left_child->keys[SECOND_KEY];

                    /* Delete borrowed key in the borrowed node (middle sibling) */
                    delete_from_node(pp_root, p_parent->p_left_child, p_parent->p_left_child->keys[SECOND_KEY]);
                }
                /* When left sibling has only one key */
                else
                {
                    if (ENABLED == delete_flag)
                    {
                        merge(pp_root, p_parent, LEFT, MIDDLE, p_current);

                        delete_flag = DISABLED;
                    }

                    /* Delete key in parent */
                    if (false == root_change)
                    {
                        delete_from_node(pp_root, p_parent, p_parent->keys[FIRST_KEY]);
                    }
                    else
                    {
                        root_change = false;
                    }
                }
            }
        }
    }
    /* When current is the root */
    else
    {
        /* Simply remove it and destroy the node */
        free(p_current);
        p_current = NULL;
        *pp_root  = NULL;
    }
}

static st_tree_node_t *get_leftmost(st_tree_node_t *p_tree_node)
{
    st_tree_node_t *p_leftmost_node;

    p_leftmost_node = p_tree_node;

    if (false == node_is_leaf(p_leftmost_node))
    {
        stack_push(&stack, (uintptr_t)p_leftmost_node);

        p_leftmost_node = get_leftmost(p_leftmost_node->p_left_child);
    }

    return p_leftmost_node;
}

static st_tree_node_t *inorder_successor(st_tree_node_t *p_tree_node, const e_key_t key_position)
{
    st_tree_node_t *p_inorder_successor;

    if (true == node_is_full(p_tree_node))
    {
        if (FIRST_KEY == key_position)
        {
            p_inorder_successor = get_leftmost(p_tree_node->p_middle_child);
        }
        else
        {
            p_inorder_successor = get_leftmost(p_tree_node->p_right_child);
        }
    }
    else
    {
        p_inorder_successor = get_leftmost(p_tree_node->p_middle_child);
    }

    return p_inorder_successor;
}

static void process_inorder_successor(st_tree_node_t **pp_root, st_tree_node_t *p_current, const e_key_t key_position)
{
    st_tree_node_t  *p_inorder_successor;
    int32_t         key_tmp;

    p_inorder_successor = inorder_successor(p_current, key_position);

    /* Swap the key to delete with inorder successor's lowest key */
    key_tmp                              = p_inorder_successor->keys[FIRST_KEY];
    p_inorder_successor->keys[FIRST_KEY] = p_current->keys[key_position];
    p_current->keys[key_position]        = key_tmp;

    delete_from_node(pp_root, p_inorder_successor, p_inorder_successor->keys[FIRST_KEY]);
}

static void delete_internal_node(st_tree_node_t **pp_root, st_tree_node_t *p_current, int32_t key_to_delete)
{
    st_tree_node_t  *p_parent;

    if (ENABLED == delete_flag)
    {
        /* When current is full (contains 2 keys) */
        if (true == node_is_full(p_current))
        {
            /* We will take inorder predecessor or inorder successor */
            stack_push(&stack, (uintptr_t)p_current);

            /* When delete the first key */
            if (key_to_delete == p_current->keys[FIRST_KEY])
            {
                process_inorder_successor(pp_root, p_current, FIRST_KEY);
            }
            /* When delete the second key */
            else
            {
                process_inorder_successor(pp_root, p_current, SECOND_KEY);
            }
        }
        /* When current has only one key */
        else
        {
            /* We will take inorder predecessor or inorder successor */
            stack_push(&stack, (uintptr_t)p_current);

            process_inorder_successor(pp_root, p_current, FIRST_KEY);
        }

        delete_flag = DISABLED;
    }
    /* When delete_flag = DISABLED */
    else
    {
        p_parent = stack_pop(&stack);

        /* When current is left child */
        if (p_current == p_parent->p_left_child)
        {
            /* Currently, in this implementation, current has only one key, never be full (contains 2 keys) */
            /* When middle sibling is full (contains 2 keys) */
            if (true == node_is_full(p_parent->p_middle_child))
            {
                /* Borrow from middle sibling */
                p_current->keys[FIRST_KEY] = p_parent->keys[FIRST_KEY];
                p_parent->keys[FIRST_KEY]  = p_parent->p_middle_child->keys[FIRST_KEY];
                node_shift(p_parent->p_middle_child, LEFT);

                /* Assign children: Shift children from middle sibling to current */
                p_current->p_middle_child                = p_parent->p_middle_child->p_left_child;
                p_parent->p_middle_child->p_left_child   = p_parent->p_middle_child->p_middle_child;
                p_parent->p_middle_child->p_middle_child = p_parent->p_middle_child->p_right_child;
                p_parent->p_middle_child->p_right_child  = NULL;
            }
            /* When middle sibling has only one key */
            else
            {
                merge(pp_root, p_parent, LEFT, MIDDLE, p_current);
            }
        }
        /* When current is middle child */
        else if (p_current == p_parent->p_middle_child)
        {
            /* Currently, in this implementation, current has only one key, never be full (contains 2 keys) */
            /* When parent is full */
            if (true == node_is_full(p_parent))
            {
                /* When right sibling is full (contains 2 keys) */
                if ( true == node_is_full(p_parent->p_right_child))
                {
                    /* Borrow from right sibling */
                    p_current->keys[FIRST_KEY] = p_parent->keys[SECOND_KEY];
                    p_parent->keys[SECOND_KEY] = p_parent->p_right_child->keys[FIRST_KEY];
                    delete_key(p_parent->p_right_child, FIRST_KEY);

                    /* Assign children: Shift children from right sibling to current  */
                    p_current->p_middle_child               = p_parent->p_right_child->p_left_child;
                    p_parent->p_right_child->p_left_child   = p_parent->p_right_child->p_middle_child;
                    p_parent->p_right_child->p_middle_child = p_parent->p_right_child->p_right_child;
                    p_parent->p_right_child->p_right_child  = NULL;
                }
                /* When left sibling is full (contains 2 keys) */
                else if (true == node_is_full(p_parent->p_left_child))
                {
                    /* Borrow from left sibling */
                    p_current->keys[FIRST_KEY] = p_parent->keys[FIRST_KEY];
                    p_parent->keys[FIRST_KEY]  = p_parent->p_left_child->keys[SECOND_KEY];
                    delete_key(p_parent->p_left_child, SECOND_KEY);

                    /* Assign children: Shift children from left sibling to current */
                    p_current->p_middle_child             = p_current->p_left_child;
                    p_current->p_left_child               = p_parent->p_left_child->p_right_child;
                    p_parent->p_left_child->p_right_child = NULL;
                }
                else
                {
                    merge(pp_root, p_parent, MIDDLE, RIGHT, p_current);
                }
            }
            /* When parent has only one key */
            else
            {
                /* When left sibling is full (contains 2 keys) */
                if (true == node_is_full(p_parent->p_left_child))
                {
                    /* Borrow from left sibling */
                    p_current->keys[FIRST_KEY] = p_parent->keys[FIRST_KEY];
                    p_parent->keys[FIRST_KEY]  = p_parent->p_left_child->keys[SECOND_KEY];
                    delete_key(p_parent->p_left_child, SECOND_KEY);

                    /* Assign children: Shift children from left sibling to current */
                    p_current->p_middle_child             = p_current->p_left_child;
                    p_current->p_left_child               = p_parent->p_left_child->p_right_child;
                    p_parent->p_left_child->p_right_child = NULL;
                }
                /* When left sibling has only one key */
                else
                {
                    merge(pp_root, p_parent, LEFT, MIDDLE, p_current);
                }
            }
        }
        /* When current is the right child */
        else
        {
            /* Currently, in this implementation, current has only one key, never be full (contains 2 keys) */
            /* When middle sibling is full (contains 2 keys) */
            if (true == node_is_full(p_parent->p_middle_child))
            {
                /* Borrow from middle sibling */
                node_shift(p_current, RIGHT);
                p_current->keys[FIRST_KEY] = p_parent->keys[SECOND_KEY];
                p_parent->keys[SECOND_KEY] = p_parent->p_middle_child->keys[SECOND_KEY];
                delete_key(p_parent->p_middle_child, SECOND_KEY);

                /* Assign children: Shift children from middle sibling to current */
                p_current->p_middle_child               = p_current->p_left_child;
                p_current->p_left_child                 = p_parent->p_middle_child->p_right_child;
                p_parent->p_middle_child->p_right_child = NULL;
            }
            /* When middle sibling has only one key */
            else
            {
                merge(pp_root, p_parent, MIDDLE, RIGHT, p_current);
            }
        }

        if (true == node_is_vacant(p_parent))
        {
            delete_internal_node(pp_root, p_parent, key_to_delete);
        }
    }

    return;
}

static void delete_from_node(st_tree_node_t **const pp_root, st_tree_node_t *const p_current, const int32_t key)
{
    if ((key == p_current->keys[FIRST_KEY]) || (key == p_current->keys[SECOND_KEY]))
    {
        /* When p_current is a leaf node */
        if (true == node_is_leaf(p_current))
        {
            /* When p_current is full (contains 2 keys), simply delete it */
            if (true == node_is_full(p_current))
            {
                if (key == p_current->keys[FIRST_KEY])
                {
                    delete_key(p_current, FIRST_KEY);
                }
                else
                {
                    delete_key(p_current, SECOND_KEY);
                }
            }
            /* When node has only one key, simply delete it then borrow or merge */
            else
            {
                delete_one_key_leaf_node(pp_root, p_current);
            }
        }
        /* When current is not a leaf */
        else
        {
            /* When current is full (contains 2 keys) */
            if (true == node_is_full(p_current))
            {
                if (key == p_current->keys[FIRST_KEY])
                {
                    /* When current's left child is vacant */
                    if (NULL == p_current->p_left_child)
                    {
                        delete_key(p_current, FIRST_KEY);

                        /* Shift all children leftward */
                        p_current->p_left_child   = p_current->p_middle_child;
                        p_current->p_middle_child = p_current->p_right_child;
                        p_current->p_right_child  = NULL;
                    }
                    else
                    {
                        delete_internal_node(pp_root, p_current, key);
                    }
                }
                else
                {
                    delete_internal_node(pp_root, p_current, key);
                }
            }
            /* When current has only one key */
            else
            {
                delete_internal_node(pp_root, p_current, key);
            }
        }
    }
    else
    {
        /* Push the parent into the stack */
        stack_push(&stack, (uintptr_t)p_current);

        if (true == key_on_the_left(key, p_current))
        {
            delete_from_node(pp_root, p_current->p_left_child, key);
        }
        else if (true == node_is_full(p_current))
        {
            if (true == key_in_the_middle(key, p_current))
            {
                delete_from_node(pp_root, p_current->p_middle_child, key);
            }
            else
            {
                delete_from_node(pp_root, p_current->p_right_child, key);
            }
        }
        else
        {
            delete_from_node(pp_root, p_current->p_middle_child, key);
        }
    }
}

void inorder_traverse(const st_tree_node_t *const p_tree_node)
{
    if (NULL != p_tree_node)
    {
        inorder_traverse(p_tree_node->p_left_child);

        printf("%d ", p_tree_node->keys[FIRST_KEY]);

        inorder_traverse(p_tree_node->p_middle_child);

        if ((-1) != p_tree_node->keys[SECOND_KEY])
        {
            printf("%d ", p_tree_node->keys[SECOND_KEY]);
        }

        inorder_traverse(p_tree_node->p_right_child);
    }
}

void preorder_traverse(const st_tree_node_t *const p_tree_node)
{
    int32_t i;

    if (NULL != p_tree_node)
    {
        for (i = 0; i < MAX_KEY; i++)
        {
            if ((-1) != p_tree_node->keys[i])
            {
                printf("%d ", p_tree_node->keys[i]);
            }
        }

        preorder_traverse(p_tree_node->p_left_child);
        preorder_traverse(p_tree_node->p_middle_child);
        preorder_traverse(p_tree_node->p_right_child);
    }
}

void postorder_traverse(const st_tree_node_t *const p_tree_node)
{
    int32_t i;

    if (NULL != p_tree_node)
    {
        postorder_traverse(p_tree_node->p_left_child);
        postorder_traverse(p_tree_node->p_middle_child);
        postorder_traverse(p_tree_node->p_right_child);

        for (i = 0; i < MAX_KEY; i++)
        {
            if ((-1) != p_tree_node->keys[i])
            {
                printf("%d ", p_tree_node->keys[i]);
            }
        }
    }
}

st_tree_node_t *search(const st_tree_node_t *const p_start_node, const int32_t searched_key)
{
    st_tree_node_t *found_node = NULL;

    if (NULL != p_start_node)
    {
        /* When node is full (contains 2 keys) */
        if (true == node_is_full(p_start_node))
        {
            if ((searched_key == p_start_node->keys[FIRST_KEY]) || (searched_key == p_start_node->keys[SECOND_KEY]))
            {
                found_node = (st_tree_node_t *)p_start_node;
            }
            else
            {
                if (true == key_on_the_left(searched_key, p_start_node))
                {
                    found_node = search(p_start_node->p_left_child, searched_key);
                }
                else if (true == key_in_the_middle(searched_key, p_start_node))
                {
                    found_node = search(p_start_node->p_middle_child, searched_key);
                }
                else
                {
                    found_node = search(p_start_node->p_right_child, searched_key);
                }
            }
        }
        /* When node has only one key */
        else
        {
            if (searched_key == p_start_node->keys[FIRST_KEY])
            {
                found_node = (st_tree_node_t *)p_start_node;
            }
            else
            {
                if (true == key_on_the_left(searched_key, p_start_node))
                {
                    found_node = search(p_start_node->p_left_child, searched_key);
                }
                else
                {
                    found_node = search(p_start_node->p_middle_child, searched_key);
                }
            }
        }
    }

    return found_node;
}

e_retcode_t insert(st_tree_node_t **const pp_root, const int32_t key)
{
    e_retcode_t ret = RET_ERRCODE_OK;

    if (NULL == pp_root)
    {
        ret = RET_ERRCODE_NG_ARGNULL;
    }
    else if ((-1) == key)
    {
        ret = RET_ERRCODE_NG_PARAM;
    }
    else if (NULL != search(*pp_root, key))
    {
        printf("Key %d is already present in the tree!\n", key);
    }
    else
    {
        insert_to_tree(pp_root, *pp_root, key, LEFT);
    }

    return ret;
}

e_retcode_t delete(st_tree_node_t **const pp_root, const int32_t key)
{
    e_retcode_t ret = RET_ERRCODE_OK;

    if (NULL == pp_root)
    {
        ret = RET_ERRCODE_NG_ARGNULL;
    }
    else if ((-1) == key)
    {
        ret = RET_ERRCODE_NG_PARAM;
    }
    else if (NULL == search(*pp_root, key))
    {
        printf("Key %d is not in the tree!\n", key);
    }
    else
    {
        delete_flag = ENABLED;
        delete_from_node(pp_root, *pp_root, key);

        /* Clear the stack */
        while (false == stack_is_empty(&stack))
        {
            (void)stack_pop(&stack);
        }
    }

    return ret;
}