#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include "types.h"

// GLOBALS.

#define DEFAULT_INTERNAL_ORDER 249
#define DEFAULT_LEAF_ORDER 31

// Page size
#define PAGESIZE 4096

// Default size of free pages
#define DEFAULT_SIZE_OF_FREE_PAGES 10

/* The queue is used to print the tree in
 * level order, starting from the root
 * printing each entire rank on a separate
 * line, finishing with the leaves.
 */
extern node* queue;

/* The user can toggle on and off the "verbose"
 * property, which causes the pointer addresses
 * to be printed out in hexadecimal notation
 * next to their corresponding keys.
 */
extern bool verbose_output;

/* File descriptor for reading and writing database file. */
extern int fd;

/* Header Page */
extern page_t header;

extern const pagenum_t PAGE_NOT_FOUND;
extern const pagenum_t FILEDESCRIPTOR_ERROR;
extern const pagenum_t PAGE_READ_ERROR;

#endif /* __GLOBALS_H__*/