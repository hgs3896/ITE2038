#ifndef __INSERT_H__
#define __INSERT_H__

#include "utils.h"
#include "file_manager.h"
#include "page_access_manager.h"

// Insertion

/*
 * Make an intenal page and return its offset.
 */
offset_t make_internal( void );

/*
 * Make a leaf page and return its offset.
 */
offset_t make_leaf( void );

/*
 * Get the index of a left page in terms of a parent page.
 */
int get_left_index( offset_t parent, offset_t left);

/*
 * Insert a record into a leaf page which has a room to save a record.
 */
offset_t insert_into_leaf( offset_t leaf, const record_t * pointer );

/*
 * Insert a record into a leaf page which has no room to save more records.
 * Therefore, first split the leaf page and call insert_into_parent() for pushing up the key.
 */
offset_t insert_into_leaf_after_splitting( offset_t root, offset_t leaf, const record_t * record);

/*
 * Insert a key into an internal page to save a record.
 * To save the key, it has to determine whether to split and whether to push a key.
 */
offset_t insert_into_parent( offset_t root, offset_t left, uint64_t key, offset_t right );

/*
 * Insert a key into a internal page which has a room to save a key.
 */
offset_t insert_into_node( offset_t root, offset_t parent, int left_index, uint64_t key, offset_t right);

/*
 * Insert a key into a internal page which has no room to save more keys.
 * Therefore, first split the internal page and call insert_into_parent() for pushing up the key recursively.
 */
offset_t insert_into_node_after_splitting( offset_t root, offset_t parent, int left_index, uint64_t key, offset_t right);

/*
 * Make a new root page for insertion of the pushup key and insert it into the root page.
 */
offset_t insert_into_new_root( offset_t left, uint64_t key, offset_t right );

/*
 * If there is no tree, make a new tree.
 */
offset_t start_new_tree( const record_t* record );

/*
 * Insert a record into the disk-based B+tree.
 */
offset_t insert_record( offset_t root, const record_t * record );

#endif