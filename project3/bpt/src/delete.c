#include "index_and_file_manager.h"

#include "utils.h"
#include "macros.h"
#include "buffer_manager.h"
#include "page_access_manager.h"

#include <stdlib.h>

// Deletion
offset_t delete_record(int table_id, offset_t root, keynum_t key) {
    offset_t key_leaf;
    record_t * key_record;

    key_record = find_record(table_id, root, key);
    key_leaf = find_leaf(table_id, root, key);
    if (key_record != NULL && key_leaf != HEADER_PAGE_OFFSET) {
        root = delete_entry(table_id, root, key_leaf, key_record);
    }
    free(key_record);
    return root;
}

offset_t delete_entry(int table_id, offset_t root, offset_t key_leaf, const record_t* record) {
    int num_keys;

    // Remove key and pointer from node.

    num_keys = remove_entry_from_node(table_id, key_leaf, record);

    /* Case:  deletion from the root.
     */

    if (key_leaf == root)
        return adjust_root(table_id, root);

    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */

    /* Delayed Merge */

    if(num_keys<=0){
        root = coalesce_nodes( table_id, root, key_leaf );
    }

    return root;
}

int remove_entry_from_node(int table_id, offset_t key_leaf_offset, const record_t* record) {
    int i, j, num_pointers, num_keys;
    buffer_frame_t *buf_leaf;
    page_t *leaf;

    buf_leaf = buf_get_frame(table_id, PGNUM(key_leaf_offset));
    leaf = buf_get_page(buf_leaf, true);
    
    // Remove the key and shift other keys accordingly.
    num_keys = getNumOfKeys(leaf);
    i = binarySearch(leaf, record->key);

    /* Removal of the Record */
    char buf[DEFAULT_RECORD_SIZE];
    for (j = i; i < num_keys - 1; i++) {
        setKey(leaf, i, getKey(leaf, i+1));
        getValue(leaf, i+1, buf);
        setValue(leaf, i, buf);
    }

    // Set the other pointers to NULL for tidiness.
    // A leaf uses the last pointer to point to the next leaf.
    setKey(leaf, num_keys - 1, 0);
    setValue(leaf, num_keys - 1, "");

    // One key fewer.
    setNumOfKeys(leaf, --num_keys);

    // Put the buffer block back to the buffer pool.
    buf_put_frame(buf_leaf);

    return num_keys;
}

offset_t adjust_root(int table_id, offset_t root_offset) {
    offset_t new_root_offset;

    /* Case: nonempty root.
     * Key and pointer have already been deleted, so nothing to be done.
     */
    buffer_frame_t *buf_root;
    page_t *root;

    buf_root = buf_get_frame(table_id, PGNUM(root_offset));
    root = buf_get_page(buf_root, false);

    if (getNumOfKeys(root) > 0){
        buf_put_frame(buf_root);
        return root_offset;
    }

    /* Case: empty root.
     */

    // If it has a child, promote the first (only) child as the new root.

    if (!isLeaf(root)) {
        new_root_offset = getOffset(root, 0);
        
        buffer_frame_t *buf_new_root = buf_get_frame(table_id, PGNUM(new_root_offset));
        setParentOffset(buf_get_page(buf_new_root, true), HEADER_PAGE_OFFSET);
        buf_put_frame(buf_new_root);
    }

    // If it is a leaf (has no children),
    // then the whole tree is empty.

    else{
        new_root_offset = HEADER_PAGE_OFFSET;
    }

    buf_free_frame(buf_root);
    buf_put_frame(buf_root);

    return new_root_offset;
}

offset_t coalesce_nodes(int table_id, offset_t root, offset_t node_to_free){
    buffer_frame_t *buf_free_pg, *buf_parent_page;
    page_t *free_page, *parent_page;
    offset_t parent_offset, neighbor_offset;
    int i, j, num_keys;
    
    buf_free_pg = buf_get_frame(table_id, PGNUM(node_to_free));
    free_page = buf_get_page(buf_free_pg, true);

    parent_offset = getParentOffset(free_page);
    
    if(parent_offset == HEADER_PAGE_OFFSET){
        buf_put_frame(buf_free_pg);
        return adjust_root(table_id, root);
    }

    buf_parent_page = buf_get_frame(table_id, PGNUM(parent_offset));
    parent_page = buf_get_page(buf_parent_page, true);

    if(isLeaf(free_page)){
        // Leaf Node
        // Move the Right Sibling Offset
        neighbor_offset = get_neighbor_offset( table_id, node_to_free );        
        if(neighbor_offset != HEADER_PAGE_OFFSET){
            buffer_frame_t *buf_neighbor_page = buf_get_frame(table_id, PGNUM(neighbor_offset));
            page_t *neighbor_page = buf_get_page(buf_neighbor_page, true);

            setOffset(neighbor_page, DEFAULT_LEAF_ORDER - 1, getOffset(free_page, DEFAULT_LEAF_ORDER - 1));

            buf_put_frame(buf_neighbor_page);
        }

        num_keys = getNumOfKeys(parent_page);

        // Find the index of the child to be deleted.
        i = 0;
        while(i<=num_keys && node_to_free != getOffset(parent_page, i))
            i++;
        // Remove it from the parent.
        for(j=i;j<num_keys;++j){
            if(j>=1) setKey(parent_page, j-1, getKey(parent_page, j));
            setOffset(parent_page, j, getOffset(parent_page, j+1));
        }
        // Clear the last case.
        setKey(parent_page, j - 1, 0);
        setOffset(parent_page, j, 0);
        // Decrease One key
        setNumOfKeys(parent_page, --num_keys);
    }else{
        // Internal Node
        offset_t only_child = getOffset(free_page, 0);
        
        buffer_frame_t* buf_child = buf_get_frame(table_id, PGNUM(only_child));
        page_t* child_page = buf_get_page(buf_child, true);

        // Find the index of the child to be deleted.
        i = 0, num_keys = getNumOfKeys(parent_page);
        while(i<=num_keys && node_to_free != getOffset(parent_page, i))
            i++;

        if(i>=1) setKey(parent_page, i-1, getKey(child_page, 0));
        setOffset(parent_page, i, only_child);
        setParentOffset(child_page, parent_offset);

        buf_put_frame(buf_child);
    }

    buf_put_frame(buf_parent_page);
    
    if(num_keys <= 0)
        root = coalesce_nodes(table_id, root, parent_offset);

    buf_free_frame(buf_free_pg);
    buf_put_frame(buf_free_pg);
    return root;
}

offset_t get_neighbor_offset(int table_id, offset_t n){
    int i, num_keys;
    
    offset_t c = n;
    buffer_frame_t *buf;
    page_t *page;

    buf = buf_get_frame(table_id, PGNUM(c));
    page = buf_get_page(buf, false);
    c = getParentOffset(page);
    buf_put_frame(buf);

    buf = buf_get_frame(table_id, PGNUM(c));
    page = buf_get_page(buf, false);

    i = 0, num_keys = getNumOfKeys(page);
    while(i<=num_keys && n != getOffset(page, i)){
        i++;
    }

    offset_t neighbor = HEADER_PAGE_OFFSET;
    if(i!=0){
        neighbor = getOffset(page, i-1);
    }
    else{
        c = getParentOffset(page);
        while(c != HEADER_PAGE_OFFSET){
            buf_put_frame(buf);
            buf = buf_get_frame(table_id, PGNUM(c));
            page = buf_get_page(buf, false);
            if(isLeaf(page)){
                neighbor = c;
                break;
            }
            num_keys = getNumOfKeys(page);
            c = getOffset(page, num_keys);
        }
    }

    buf_put_frame(buf);
    return neighbor;
}