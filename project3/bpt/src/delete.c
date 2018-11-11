#include "index_manager.h"
#include "file_manager.h"
#include "page_access_manager.h"

#include <stdlib.h>

// Deletion
offset_t delete_record(int table_id, offset_t root, keynum_t key ) {
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

offset_t delete_entry(int table_id, offset_t root, offset_t key_leaf, const record_t* record ) {
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

int remove_entry_from_node(int table_id, offset_t key_leaf_offset, const record_t* record ) {
    int i, j, num_pointers, num_keys;
    page_t leaf;
    file_read_page(table_id, PGNUM(key_leaf_offset), &leaf);
    
    // Remove the key and shift other keys accordingly.
    num_keys = getNumOfKeys(&leaf);
    i = binarySearch(&leaf, record->key);

    /* Removal of the Record */
    char buf[120];
    for (j = i; i < num_keys - 1; i++) {
        setKey(&leaf, i, getKey(&leaf, i+1));
        getValue(&leaf, i+1, buf);
        setValue(&leaf, i, buf);
    }

    // Set the other pointers to NULL for tidiness.
    // A leaf uses the last pointer to point to the next leaf.
    setKey(&leaf, num_keys - 1, 0);
    setValue(&leaf, num_keys - 1, "");

    // One key fewer.
    setNumOfKeys(&leaf, --num_keys);
    
    // Apply changes to the file.
    file_write_page(table_id, PGNUM(key_leaf_offset), &leaf);

    return num_keys;
}

offset_t adjust_root(int table_id, offset_t root_offset ) {
    offset_t new_root_offset;

    /* Case: nonempty root.
     * Key and pointer have already been deleted, so nothing to be done.
     */
    page_t root;
    file_read_page(table_id, PGNUM(root_offset), &root);

    if (getNumOfKeys(&root) > 0)
        return root_offset;

    /* Case: empty root.
     */

    // If it has a child, promote the first (only) child as the new root.

    if (!isLeaf(&root)) {
        new_root_offset = getOffset(&root, 0);
        
        page_t new_root;
        file_read_page(table_id, PGNUM(new_root_offset), &new_root);
        setParentOffset(&new_root, HEADER_PAGE_OFFSET);
        file_write_page(table_id, PGNUM(new_root_offset), &new_root);
    }

    // If it is a leaf (has no children),
    // then the whole tree is empty.

    else{
        new_root_offset = HEADER_PAGE_OFFSET;
    }

    file_free_page(table_id, PGNUM(root_offset));

    return new_root_offset;
}

offset_t coalesce_nodes(int table_id, offset_t root, offset_t node_to_free ){
    page_t free_page, parent_page;
    offset_t parent_offset, neighbor_offset;
    int i, j, num_keys;
    
    file_read_page(table_id, PGNUM(node_to_free), &free_page);
    parent_offset = getParentOffset(&free_page);
    
    if(parent_offset == HEADER_PAGE_OFFSET){
        return adjust_root(table_id, root);
    }

    file_read_page(table_id, PGNUM(parent_offset), &parent_page);

    if(isLeaf(&free_page)){
        // Leaf Node
        // Move the Right Sibling Offset
        neighbor_offset = get_neighbor_offset( table_id, node_to_free );        
        if(neighbor_offset != HEADER_PAGE_OFFSET){
            page_t neighbor_page;
            file_read_page(table_id, PGNUM(neighbor_offset), &neighbor_page);
            setOffset(&neighbor_page, DEFAULT_LEAF_ORDER - 1, getOffset(&free_page, DEFAULT_LEAF_ORDER - 1));
            file_write_page(table_id, PGNUM(neighbor_offset), &neighbor_page);
        }

        num_keys = getNumOfKeys(&parent_page);

        // Find the index of the child to be deleted.
        i = 0;
        while(i<=num_keys && node_to_free != getOffset(&parent_page, i))
            i++;
        // Remove it from the parent.
        for(j=i;j<num_keys;++j){
            if(j>=1) setKey(&parent_page, j-1, getKey(&parent_page, j));
            setOffset(&parent_page, j, getOffset(&parent_page, j+1));
        }
        // Clear the last case.
        setKey(&parent_page, j - 1, 0);
        setOffset(&parent_page, j, 0);
        // Decrease One key
        setNumOfKeys(&parent_page, --num_keys);
    }else{
        // Internal Node
        page_t buf;
        offset_t only_child = getOffset(&free_page, 0);
        
        file_read_page(table_id, PGNUM(only_child), &buf);

        // Find the index of the child to be deleted.
        i = 0, num_keys = getNumOfKeys(&parent_page);
        while(i<=num_keys && node_to_free != getOffset(&parent_page, i))
            i++;

        if(i>=1) setKey(&parent_page, i-1, getKey(&buf, 0));
        setOffset(&parent_page, i, only_child);
        setParentOffset(&buf, parent_offset);

        file_write_page(PGNUM(only_child), &buf);        
    }

    file_write_page(table_id, PGNUM(parent_offset), &parent_page);
    
    if(num_keys <= 0)
        root = coalesce_nodes(table_id, root, parent_offset );

    file_free_page(table_id, PGNUM(node_to_free));
    return root;
}

offset_t get_neighbor_offset(int table_id, offset_t n ){
    int i, num_keys;
    page_t buf;
    offset_t c = n;
    file_read_page(table_id, PGNUM(c), &buf);

    c = getParentOffset(&buf);
    file_read_page(table_id, PGNUM(c), &buf);
    i = 0, num_keys = getNumOfKeys(&buf);
    while(i<=num_keys && n != getOffset(&buf, i)){
        i++;
    }
    if(i!=0)
        return getOffset(&buf, i-1);
    else{
        c = getParentOffset(&buf);
        while(c != HEADER_PAGE_OFFSET){
            file_read_page(table_id, PGNUM(c), &buf);
            if(isLeaf(&buf))
                return c;
            num_keys = getNumOfKeys(&buf);
            c = getOffset(&buf, num_keys);
        }
    }

    return HEADER_PAGE_OFFSET;    
}