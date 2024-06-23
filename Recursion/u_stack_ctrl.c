#include "u_stack_ctrl.h"

st_stack_node_t *create_stack_node(uintptr_t data)
{
    st_stack_node_t *stack_node;

    stack_node = (st_stack_node_t *)malloc(sizeof(st_stack_node_t));
    if (NULL != stack_node)
    {
        stack_node->data = data;
        stack_node->next = NULL;
    }

    return stack_node;
}

bool_t stack_is_empty(st_stack_t *p_stack)
{
    return (NULL == p_stack->top);
}

void stack_push(st_stack_t *p_stack, uintptr_t data)
{
    st_stack_node_t *p_stack_node = create_stack_node(data);

    p_stack_node->next = p_stack->top;
    p_stack->top       = p_stack_node;
}

st_tree_node_t *stack_pop(st_stack_t *p_stack)
{
    st_stack_node_t *popped_node;
    st_tree_node_t  *p_tree_node = NULL;

    if (false == stack_is_empty(p_stack))
    {
        popped_node = p_stack->top;
        p_tree_node = (st_tree_node_t *)popped_node->data;

        /* Update top */
        p_stack->top = p_stack->top->next;

        /* Unlink and destroy */
        popped_node->next = NULL;
        free(popped_node);
    }

    return p_tree_node;
}

st_stack_node_t *stack_top(st_stack_t *p_stack)
{
    return p_stack->top;
}