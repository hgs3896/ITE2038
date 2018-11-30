#include "index_and_file_manager.h"

#include "utils.h"
#include "macros.h"
#include "wrapper_funcs.h"
#include "buffer_manager.h"

// Deletion
offset_t delete_record(int table_id, offset_t root, record_key_t key) {
	auto key_record = find_record(table_id, root, key);
    auto key_leaf = find_leaf(table_id, root, key);
    if (key_record != NULL && key_leaf != HEADER_PAGE_OFFSET) {
        root = delete_entry(table_id, root, key_leaf, key_record);
    }
    delete key_record;
    return root;
}

offset_t delete_entry(int table_id, offset_t root, offset_t key_leaf, const record_t* record) {
	// Remove key and pointer from node.
	auto num_keys = remove_entry_from_node(table_id, key_leaf, record);

    /* Case:  deletion from the root.
     */

    if (key_leaf == root)
        return adjust_root(table_id, root);

    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */

    /* Delayed Merge */

	if ( num_keys <= 0 )
		root = coalesce_nodes(table_id, root, key_leaf);

    return root;
}

int remove_entry_from_node(int table_id, offset_t key_leaf_offset, const record_t* record) {
	auto i = 0, j = 0, num_pointers = 0, num_keys = 0;

    auto buf_leaf = BufferManager::get_frame(table_id, PGNUM(key_leaf_offset));
    auto leaf = BufferManager::get_page(buf_leaf, true);
    
    // Remove the key and shift other keys accordingly.
    num_keys = leaf->getNumOfKeys();
    i = leaf->binarySearch(record->key);
	
    /* Removal of the Record */
	const auto num_cols = getNumOfCols(table_id);
	auto buf = new record_val_t[num_cols - 1];
    for (j = i; i < num_keys - 1; i++) {
		leaf->setKey(i, leaf->getKey(i+1));
		leaf->getValues(i+1, buf, num_cols);
		leaf->setValues(i, buf, num_cols);
    }
	delete[] buf;

    // Set the other pointers to NULL for tidiness.
    // A leaf uses the last pointer to point to the next leaf.
	leaf->setKey(num_keys - 1, 0);
	leaf->setValues(num_keys - 1, nullptr);

    // One key fewer.
	leaf->setNumOfKeys(--num_keys);

    // Put the buffer block back to the buffer pool.
    BufferManager::put_frame(buf_leaf);

    return num_keys;
}

offset_t adjust_root(int table_id, offset_t root_offset) {
    offset_t new_root_offset;

    /* Case: nonempty root.
     * Key and pointer have already been deleted, so nothing to be done.
     */

    auto buf_root = BufferManager::get_frame(table_id, PGNUM(root_offset));
    auto root = BufferManager::get_page(buf_root, false);

	auto num_of_keys = root->getNumOfKeys();
	auto isLeaf = root->isLeaf();

    if ( num_of_keys > 0){
		BufferManager::put_frame(buf_root);
		new_root_offset = root_offset;
	}
	else
	{
		/* Case: empty root.
		*/

		// If it has a child, promote the first (only) child as the new root.

		if ( !isLeaf )
		{
			new_root_offset = root->getOffset(0);
			BufferManager::put_frame(buf_root);

			auto buf_new_root = BufferManager::get_frame(table_id, PGNUM(new_root_offset));
			BufferManager::get_page(buf_new_root, true)->setParentOffset(HEADER_PAGE_OFFSET);
			BufferManager::put_frame(buf_new_root);
		}

		// If it is a leaf (has no children),
		// then the whole tree is empty.

		else
		{
			BufferManager::put_frame(buf_root);
			new_root_offset = HEADER_PAGE_OFFSET;
		}

		BufferManager::buf_free_page(table_id, root_offset);
	}
    return new_root_offset;
}

offset_t coalesce_nodes(int table_id, offset_t root, offset_t node_to_free){
	// Get some informaiton from the node to be free
    auto buf_free_pg = BufferManager::get_frame(table_id, PGNUM(node_to_free));
    auto free_page = BufferManager::get_page(buf_free_pg, true);

    auto parent_offset = free_page->getParentOffset();
	auto isLeaf = free_page->isLeaf();
	auto nextPageOffset = free_page->getOffset(DEFAULT_LEAF_ORDER - 1);
    
	BufferManager::put_frame(buf_free_pg);

	// If the root is to be deleted,
    if(parent_offset == HEADER_PAGE_OFFSET){
        return root = adjust_root(table_id, root);
    }
	
	// Get some informaiton from the parent node
	auto buf_parent_page = BufferManager::get_frame(table_id, PGNUM(parent_offset));
	auto parent_page = BufferManager::get_page(buf_parent_page, true);
	auto num_keys = parent_page->getNumOfKeys();

	// If the number of keys left in the parent node is more than one, then remove the pair of key and offset.
	if ( num_keys > 1 )
	{
		// If the page we want to remove is leaf node,
		if ( isLeaf )
		{
			// Make the neighbor page's right sibling offset point to that of the free page.
			auto neighbor_offset = get_neighbor_offset(table_id, node_to_free);
			if ( neighbor_offset != HEADER_PAGE_OFFSET )
			{
				auto buf_neighbor_page = BufferManager::get_frame(table_id, PGNUM(neighbor_offset));
				auto neighbor_page = BufferManager::get_page(buf_neighbor_page, true);

				neighbor_page->setOffset(DEFAULT_LEAF_ORDER - 1, nextPageOffset);

				BufferManager::put_frame(buf_neighbor_page);
			}
		}

		auto i = 0;
		// Find the index of the child page offset to take it out
		while ( i <= num_keys && node_to_free != parent_page->getOffset(i) )
			i++;

		// Remove the offset from the parent page.
		auto j = 0;
		for ( j = i; j < num_keys; ++j )
		{
			if ( j >= 1 ) parent_page->setKey(j - 1, parent_page->getKey(j));
			parent_page->setOffset(j, parent_page->getOffset(j + 1));
		}

		// Clean the last one for tideness.
		parent_page->setKey(j - 1, 0);
		parent_page->setOffset(j, 0);

		// Decrease One key
		parent_page->setNumOfKeys(--num_keys);
	}
	else // If the number of keys left in the parent node is less than or equal to one, then free it until every child becomes empty.
	{
		auto num_keys_left = 0;
		auto left_offset = parent_page->getOffset(0);
		auto right_offset = parent_page->getOffset(1);
		auto delete_first_node = node_to_free == left_offset;
		
		auto buf_another = BufferManager::get_frame(table_id, PGNUM(parent_page->getOffset(delete_first_node)));
		auto another_page = BufferManager::get_page(buf_another, false);
		num_keys_left = another_page->getNumOfKeys();
		BufferManager::put_frame(buf_another);

		// If the page we want to remove is leaf node,
		if ( isLeaf )
		{
			if ( num_keys_left == 0 )
			{
				// Make the neighbor page's right sibling offset point to that of the free page.
				auto neighbor_offset = get_neighbor_offset(table_id, left_offset);
				if ( neighbor_offset != HEADER_PAGE_OFFSET )
				{
					auto buf_neighbor_page = BufferManager::get_frame(table_id, PGNUM(neighbor_offset));
					auto neighbor_page = BufferManager::get_page(buf_neighbor_page, true);

					neighbor_page->setOffset(DEFAULT_LEAF_ORDER - 1, nextPageOffset);

					BufferManager::put_frame(buf_neighbor_page);
				}
			}

			// Push it back to the buffer
			BufferManager::put_frame(buf_parent_page);

			BufferManager::buf_free_page(table_id, parent_page->getOffset(0));
			BufferManager::buf_free_page(table_id, parent_page->getOffset(1));
			
			// Merge the tree
			root = coalesce_nodes(table_id, root, parent_offset);
		}
	}

	

	// Put it into the Free Page List
	BufferManager::buf_free_page(table_id, node_to_free);

    return root;
}

offset_t get_neighbor_offset(int table_id, offset_t n){
    int idx, num_keys;
    
	offset_t neighbor = HEADER_PAGE_OFFSET;
    offset_t p, c = n;

    auto buf = BufferManager::get_frame(table_id, PGNUM(c));
    auto page = BufferManager::get_page(buf, false);
	p = page->getParentOffset();
    BufferManager::put_frame(buf);

	while(true){
		if ( p == HEADER_PAGE_OFFSET )
		{
			// The neighbor node has not been found.
			neighbor = HEADER_PAGE_OFFSET;
			break;
		}
		
		buf = BufferManager::get_frame(table_id, PGNUM(p));
		page = BufferManager::get_page(buf, false);

		idx = 0, num_keys = page->getNumOfKeys();
		while ( idx <= num_keys && c != page->getOffset(idx) )
			idx++;

		if ( idx != 0 )
		{
			c = page->getOffset(idx - 1);
			BufferManager::put_frame(buf);
			break;
		}

		c = p;
		p = page->getParentOffset();
		BufferManager::put_frame(buf);
	}
	
	while ( true )
	{
		buf = BufferManager::get_frame(table_id, PGNUM(c));
		page = BufferManager::get_page(buf, false);

		if ( page->isLeaf() )
		{
			BufferManager::put_frame(buf);
			neighbor = c;
			break;
		}

		// Move to the rightmost node.
		c = page->getOffset(page->getNumOfKeys());
		BufferManager::put_frame(buf);
	}

    return neighbor;
}