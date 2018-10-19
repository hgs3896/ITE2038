#include "bpt.h"

// UTILITIES FUNCTION DEFINITIONS

void usage_2( void ) {
    printf("Enter any of the following commands after the prompt > :\n"
    "\ti <k>  -- Insert <k> (an integer) as both key and value).\n"
    "\tf <k>  -- Find the value under key <k>.\n"
    "\tp <k> -- Print the path from the root to key k and its associated "
           "value.\n"
    "\tr <k1> <k2> -- Print the keys and values found in the range "
            "[<k1>, <k2>\n"
    "\td <k>  -- Delete key <k> and its associated value.\n"
    "\tx -- Destroy the whole tree.  Start again with an empty tree of the "
           "same order.\n"
    "\tt -- Print the B+ tree.\n"
    "\tl -- Print the keys of the leaves (bottom row of the tree).\n"
    "\tv -- Toggle output of pointer addresses (\"verbose\") in tree and "
           "leaves.\n"
    "\tq -- Quit. (Or use Ctl-D.)\n"
    "\t? -- Print this help message.\n");
}

// OUTPUT FUNCTION DEFINITIONS.

void enqueue( offset_t new_node_offset ) {
    node * c, * new_node = malloc(sizeof node);
    
    new_node->offset = new_node_offset;
    new_node->next = NULL;

    if ( queue == NULL ) {
        queue = new_node;
        queue->next = NULL;
    }
    else {
        c = queue;
        while(c->next != NULL) {
            c = c->next;
        }
        c->next = new_node;
    }
}

offset_t dequeue( void ) {
    node * n = queue;
    queue = queue->next;
    n->next = NULL;
    offset_t ret = n->offset;
    free(n);
    return ret;
}