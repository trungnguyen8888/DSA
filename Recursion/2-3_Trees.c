/*
    Let's code a 2-3 tree: 20, 30, 40, 50, 60, 11, 15, 70, 80
    Each node has [ key[2] | *left_child | *middle_child | *right_child | is_root ]

    Output:
                        [30| ]
                    /          \
              [15| ]            [50|70]
             /      \          /    |   \
          [11| ]  [20| ]  [40| ] [60| ]  [80| ]

    Notes: This implementation is just a demonstration, excluding error handling
*/

#include "u_stack_ctrl.h"
#include "u_util.h"

static void print_out_traversals(const st_tree_node_t *const p_tree_node);

static void print_out_traversals(const st_tree_node_t *const p_tree_node)
{
    printf("Preorder:\t");
    preorder_traverse(p_tree_node);
    printf("\n");

    printf("Inorder:\t");
    inorder_traverse(p_tree_node);
    printf("\n");

    printf("Postorder:\t");
    postorder_traverse(p_tree_node);
    printf("\n\n");
}

int32_t process()
{
    int32_t         i;
    int32_t         keys[] =  { 24, 35, 40, 50, 60, 18, 22, 70, 80, 11, 14, 3, 20, 30, 46, 66, 90, 8, 5, 13, 28, 26, 32, -2, 2, 7 };
    st_tree_node_t  *root;
    int32_t         key_to_search;
    int32_t         key_to_delete;
    st_tree_node_t  *found_node;

    /* Initialize */
    root = NULL;
    found_node = NULL;

    /* Insert values */
    for (i = 0; i < ARRAY_SIZE(keys); i++)
    {
        insert(&root, keys[i]);
    }

    /* Traverse the tree */
    print_out_traversals(root);

    /* Search for a key */
    printf("Input the key to search: ");
    scanf("%d", &key_to_search);
    found_node = search(root, key_to_search);
    if (NULL != found_node)
    {
        printf("Key %d is found!\n", key_to_search);
    }
    else
    {
        printf("Key %d is not present on the tree!\n", key_to_search);
    }

    /* Delete keys */
    printf("Input the key to delete: ");
    scanf("%d", &key_to_delete);
    delete(&root, key_to_delete);

    return 0;
}

int32_t main()
{
    return process();
}