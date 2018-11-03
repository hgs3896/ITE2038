#ifndef __DELETE_H__
#define __DELETE_H__

#include "utils.h"
#include "file_manager.h"
#include "page_access_manager.h"

// Deletion
offset_t delete_record( offset_t root, keynum_t key );
offset_t delete_entry( offset_t root, offset_t key_leaf, const record_t* record );
int remove_entry_from_node( offset_t key_leaf, const record_t* record );
offset_t adjust_root( offset_t root );
offset_t coalesce_nodes( offset_t root, offset_t node_to_free );
offset_t get_neighbor_offset( offset_t n );

#endif