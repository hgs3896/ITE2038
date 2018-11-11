#ifndef __UTILS_H__
#define __UTILS_H__

#include "types.h"

// Utility

/*
 * Print usage about the commands used to control the disk-based B+tree.
 */
void usage(void);
int cut( int length );

/* Queue operations for printing and traversing the B+ tree in the level order*/
void enqueue( offset_t new_node, int depth );
offset_t dequeue( int *depth );

void print_leaves( int table_id, offset_t root );
void print_tree( int table_id, offset_t root );
int find_range( int table_id, offset_t root, keynum_t key_start, keynum_t key_end, record_t records[] );
void find_and_print_range( int table_id, offset_t root, keynum_t key_start, keynum_t key_end);

#endif