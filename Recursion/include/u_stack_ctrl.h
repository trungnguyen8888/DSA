#ifndef STACK_CTRL_H
#define STACK_CTRL_H

#include "u_errors.h"

struct st_stack_node
{
    uintptr_t               data;
    struct st_stack_node    *next;
};

struct st_stack
{
    st_stack_node_t         *top;
};

st_stack_node_t *create_stack_node(uintptr_t data);
bool_t stack_is_empty(st_stack_t *p_stack);
void stack_push(st_stack_t *p_stack, uintptr_t data);
st_tree_node_t *stack_pop(st_stack_t *p_stack);
st_stack_node_t *stack_top(st_stack_t *p_stack);

#endif