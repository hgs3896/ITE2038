#include "bpt.h"

// GLOBALS

/*
 * The queue is used to print the tree in level order,
 * starting from the root printing each entire rank on a separate line,
 * finishing with the leaves.
 */
node* queue;

/*
 * File Descriptor for R/W
 */
int fd;

/*
 * Cached Header Page
 */
page_t header;