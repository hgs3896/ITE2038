#ifndef __INDEX_MANAGER_H__
#define __INDEX_MANAGER_H__

#include "types.h"

// GLOBALS

/*
 * The queue is used to print the tree in level order,
 * starting from the root printing each entire rank on a separate line,
 * finishing with the leaves.
 */
extern node* queue;

/*
 * Cached Header Page
 */
extern page_t header;

// File Manager APIs

/*
 * Initialize buffer pool with given number and buffer manager.
 */
int init_db(int buf_num);

/*
 * Destroy buffer manager.
 */
int shutdown_db(void);

/*
 * Open existing data file using ‘pathname’ or create one if not existed.
 * If success, return table_id.
 */
int open_table(char * pathname);

/*
 * Write the pages relating to this table to disk and close the table.
 */
int close_table(int table_id);

/*
    Find the record containing input ‘key’.
    If found matching ‘key’, return matched ‘value’ string. Otherwise, return NULL.
*/
char* find(int table_id, keynum_t key);

/*
 *  Insert input ‘key/value’ (record) to data file at the right place.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int insert(int table_id, keynum_t key, char * value);

/*
 *  Find the matching record and delete it if found.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int delete(int table_id, keynum_t key);

/*
 *  Allocate an on-disk page from the free page list
 */
pagenum_t file_alloc_page(int table_id);

/*
 *  Free an on-disk page to the free page list
 */
void file_free_page(int table_id, pagenum_t pagenum);


// Search

/* Find where the given key is located and return the leaf page offset which it has the key.*/
offset_t find_leaf(int table_id, offset_t root, keynum_t key);

// find_record() needs to be freed after the call
record_t * find_record(int table_id, offset_t root, keynum_t key);

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
int get_left_index(int table_id, offset_t parent, offset_t left);

/*
 * Insert a record into a leaf page which has a room to save a record.
 */
offset_t insert_into_leaf(int table_id, offset_t leaf, const record_t * pointer);

/*
 * Insert a record into a leaf page which has no room to save more records.
 * Therefore, first split the leaf page and call insert_into_parent() for pushing up the key.
 */
offset_t insert_into_leaf_after_splitting(int table_id, offset_t root, offset_t leaf, const record_t * record);

/*
 * Insert a key into an internal page to save a record.
 * To save the key, it has to determine whether to split and whether to push a key.
 */
offset_t insert_into_parent(int table_id, offset_t root, offset_t left, uint64_t key, offset_t right);

/*
 * Insert a key into a internal page which has a room to save a key.
 */
offset_t insert_into_node(int table_id, offset_t root, offset_t parent, int left_index, uint64_t key, offset_t right);

/*
 * Insert a key into a internal page which has no room to save more keys.
 * Therefore, first split the internal page and call insert_into_parent() for pushing up the key recursively.
 */
offset_t insert_into_node_after_splitting(int table_id, offset_t root, offset_t parent, int left_index, uint64_t key, offset_t right);

/*
 * Make a new root page for insertion of the pushup key and insert it into the root page.
 */
offset_t insert_into_new_root(int table_id, offset_t left, uint64_t key, offset_t right);

/*
 * If there is no tree, make a new tree.
 */
offset_t start_new_tree(int table_id, const record_t* record);

/*
 * Insert a record into the disk-based B+tree.
 */
offset_t insert_record(int table_id, offset_t root, const record_t * record);

// Deletion

offset_t delete_record(int table_id, offset_t root, keynum_t key);

offset_t delete_entry(int table_id, offset_t root, offset_t key_leaf, const record_t* record);

int remove_entry_from_node(int table_id, offset_t key_leaf, const record_t* record);

offset_t adjust_root(int table_id, offset_t root);

offset_t coalesce_nodes(int table_id, offset_t root, offset_t node_to_free);

offset_t get_neighbor_offset(int table_id, offset_t n);

#endif