#ifndef __INDEX_AND_FILE_MANAGER_H__
#define __INDEX_AND_FILE_MANAGER_H__

#include "types.h"

// GLOBALS

// Search

/* Find where the given key is located and return the leaf page offset which it has the key.*/
offset_t find_leaf(int table_id, offset_t root, record_key_t key);

// find_record() needs to be freed after the call
record_t * find_record(int table_id, offset_t root, record_key_t key);

// Insertion

/*
 * Make an intenal page and return its offset.
 */
offset_t make_internal(int table_id);

/*
 * Make a leaf page and return its offset.
 */
offset_t make_leaf(int table_id);

/*
 * Get the index of a left page in terms of a parent page.
 */
int get_left_index(int table_id, offset_t parent_offset, offset_t left_offset);

/*
 * Insert a record into the disk-based B+tree.
 */
offset_t insert_record(int table_id, offset_t root, const record_t* record);

/*
 * Insert a record into a leaf page which has a room to save a record.
 */
offset_t insert_into_leaf(int table_id, offset_t leaf_offset, const record_t* record);

/*
 * Insert a record into a leaf page which has no room to save more records.
 * Therefore, first split the leaf page and call insert_into_parent() for pushing up the key.
 */
offset_t insert_into_leaf_after_splitting(int table_id, offset_t root_offset, offset_t leaf_offset, const record_t* record);

/*
 * Insert a key into an internal page to save a record.
 * To save the key, it has to determine whether to split and whether to push a key.
 */
offset_t insert_into_parent(int table_id, offset_t root_offset, offset_t left_offset, uint64_t key, offset_t right_offset);

/*
 * Insert a key into a internal page which has a room to save a key.
 */
offset_t insert_into_node(int table_id, offset_t root_offset, offset_t parent_offset, int left_index, uint64_t key, offset_t right_offset);

/*
 * Insert a key into a internal page which has no room to save more keys.
 * Therefore, first split the internal page and call insert_into_parent() for pushing up the key recursively.
 */
offset_t insert_into_node_after_splitting(int table_id, offset_t root_offset, offset_t parent_offset, int left_index, uint64_t key, offset_t right_offset);

/*
 * Make a new root page for insertion of the pushup key and insert it into the root page.
 */
offset_t insert_into_new_root(int table_id, offset_t left_offset, uint64_t key, offset_t right_offset);

/*
 * If there is no tree, make a new tree.
 */
offset_t start_new_tree(int table_id, const record_t* record);

// Deletion

offset_t delete_record(int table_id, offset_t root, record_key_t key);

offset_t delete_entry(int table_id, offset_t root, offset_t key_leaf, const record_t* record);

int remove_entry_from_node(int table_id, offset_t key_leaf_offset, const record_t* record);

offset_t adjust_root(int table_id, offset_t root_offset);

offset_t coalesce_nodes(int table_id, offset_t root, offset_t node_to_free);

offset_t get_neighbor_offset(int table_id, offset_t n);

#endif