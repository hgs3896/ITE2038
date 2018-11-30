#include "index_and_file_manager.h"

#include "utils.h"
#include "macros.h"
#include "wrapper_funcs.h"
#include "buffer_manager.h"

#include <stdlib.h>
#include <string.h>

/*
 * Make an intenal page and return its offset.
 */
offset_t make_internal(int table_id) {
    // Allocate one page from the free page list.
    offset_t new_page_offset = BufferManager::buf_alloc_page(table_id);
    BufferBlock *buf = BufferManager::get_frame(table_id, PGNUM(new_page_offset));
    Page *internal_node = BufferManager::get_page(buf, true);

    internal_node->clear();
	internal_node->setInternal();

    BufferManager::put_frame(buf);
    return new_page_offset;
}

/*
 * Make a leaf page and return its offset.
 */
offset_t make_leaf( int table_id ) {
    // Allocate one page from the free page list.
    offset_t new_page_offset = BufferManager::buf_alloc_page(table_id);
    BufferBlock *buf = BufferManager::get_frame(table_id, PGNUM(new_page_offset));
    Page *leaf_node = BufferManager::get_page(buf, true);

	leaf_node->clear();
	leaf_node->setLeaf();

    BufferManager::put_frame(buf);
    return new_page_offset;
}

/*
 * Get the index of a left page in terms of a parent page.
 */
int get_left_index(int table_id, offset_t parent_offset, offset_t left_offset) {
    BufferBlock* buf = NULL;
    buf = BufferManager::get_frame(table_id, PGNUM(parent_offset));
    assert(buf != NULL);

    Page* parent = BufferManager::get_page(buf, false);

    int num_keys = parent->getNumOfKeys();
    int left_index = 0;
    while (left_index <= num_keys && parent->getOffset(left_index) != left_offset)
        left_index++;

    BufferManager::put_frame(buf);
    return left_index;
}

/* Master insertion function.
* Inserts a key and an associated value into
* the B+ tree, causing the tree to be adjusted
* however necessary to maintain the B+ tree
* properties.
*/

offset_t insert_record(int table_id, offset_t root_offset, const record_t* record)
{
	record_t * pointer;
	BufferBlock *buf_leaf;
	Page *leaf;

	/* The current implementation ignores duplicates.
	 */

	 // Duplicated
	if ( (pointer = find_record(table_id, root_offset, record->key)) != NULL )
	{
		free(pointer);
		return root_offset;
	}

	/* Case: the tree does not exist yet.
	 * Start a new tree.
	 */

	if ( root_offset == HEADER_PAGE_OFFSET )
		return start_new_tree(table_id, record);

	offset_t leaf_offset = find_leaf(table_id, root_offset, record->key);

	if ( leaf_offset == HEADER_PAGE_OFFSET )
		return root_offset;

	/* Case: the tree already exists.
	 * (Rest of function body.)
	 */

	 /* Case: leaf has room for key and pointer.
	  */

	buf_leaf = BufferManager::get_frame(table_id, PGNUM(leaf_offset));
	assert(buf_leaf != NULL);
	leaf = BufferManager::get_page(buf_leaf, false);
	int num_keys = leaf->getNumOfKeys();
	BufferManager::put_frame(buf_leaf);

	if ( num_keys < DEFAULT_LEAF_ORDER - 1 )
	{
		leaf_offset = insert_into_leaf(table_id, leaf_offset, record);
		return root_offset;
	}

	/* Case:  leaf must be split.
	 */

	return insert_into_leaf_after_splitting(table_id, root_offset, leaf_offset, record);
}

/*
 * Insert a record into a leaf page which has a room to save a record.
 */
offset_t insert_into_leaf(int table_id, offset_t leaf_offset, const record_t* record) {
    int i, insertion_point, num_keys;

	const auto num_cols = getNumOfCols(table_id);

    BufferBlock* buf = NULL;
    buf = BufferManager::get_frame(table_id, PGNUM(leaf_offset));
    assert(buf != NULL);

    Page* leaf = BufferManager::get_page(buf, true);

    /* Load the metatdata. */
    num_keys = leaf->getNumOfKeys();

    /* Find the insertion point by binary search. */
    if (record->key <  leaf->getKey(0))
        insertion_point = 0;
    else {
        int index = leaf->binaryRangeSearch(record->key);
        if (index == INVALID_KEY){
            BufferManager::put_frame(buf);
            return INVALID_KEY;
        }

        insertion_point = index + 1;
    }

	record_t temp;

    /* Shifting */
    for (i = num_keys; i > insertion_point; i--) {
        leaf->setKey(i, leaf->getKey(i - 1));
        leaf->getValues(i - 1, temp.values, num_cols);
        leaf->setValues(i, temp.values, num_cols);
    }

    /* Insertion */
    leaf->setKey(insertion_point, record->key);
    leaf->setValues(insertion_point, record->values, num_cols);

    /* Set Metadata */
	leaf->setNumOfKeys(leaf->getNumOfKeys() + 1);

    /* Set it dirty. */
    BufferManager::put_frame(buf);
    return leaf_offset;
}

/*
 * Insert a record into a leaf page which has no room to save more records.
 * Therefore, first split the leaf page and call insert_into_parent() for pushing up the key.
 */
offset_t insert_into_leaf_after_splitting(int table_id, offset_t root_offset, offset_t leaf_offset, const record_t* record) {
    BufferBlock *buf_leaf, *buf_new_leaf;
    Page *leaf, *new_leaf;
    offset_t new_leaf_offset;
    record_key_t temp_keys[DEFAULT_LEAF_ORDER];
	record_val_t temp_values[DEFAULT_LEAF_ORDER][MAX_NUM_COLUMNS];
    int insertion_index, split, new_key, i, j, num_keys;
	const auto num_cols = getNumOfCols(table_id);

    new_leaf_offset = make_leaf(table_id);

    buf_leaf = BufferManager::get_frame(table_id, PGNUM(leaf_offset));
    buf_new_leaf = BufferManager::get_frame(table_id, PGNUM(new_leaf_offset));
    assert(buf_leaf != NULL);
    assert(buf_new_leaf != NULL);

    leaf = BufferManager::get_page(buf_leaf, true);
    new_leaf = BufferManager::get_page(buf_new_leaf, true);

    /* Find the Insertion Point */
    if (record->key < leaf->getKey(0))
        insertion_index = 0;
    else
        insertion_index = leaf->binaryRangeSearch(record->key) + 1;

    /* Move to temporary space. */
    for (i = 0, j = 0, num_keys = leaf->getNumOfKeys(); i < num_keys; i++, j++) {
        if (j == insertion_index) j++;
        temp_keys[j] = leaf->getKey(i);
        leaf->getValues(i, temp_values[j], num_cols);
    }

    /* Insertion */
    temp_keys[insertion_index] = record->key;
    std::memcpy(temp_values[insertion_index], record->values, DEFAULT_VALUE_SIZE);

    /* Split */
    split = cut(DEFAULT_LEAF_ORDER - 1);

    /* Distribute the records into two split leaf nodes. */
    for (i = 0, num_keys = 0; i < split; i++) {
		leaf->setValues(i, temp_values[i], num_cols);
		leaf->setKey(i, temp_keys[i]);
		leaf->setNumOfKeys(++num_keys);
    }

    for (i = split, j = 0, num_keys = 0; i < DEFAULT_LEAF_ORDER; i++, j++) {
		new_leaf->setValues(j, temp_values[i], num_cols);
		new_leaf->setKey(j, temp_keys[i]);
		new_leaf->setNumOfKeys(++num_keys);
    }

    /* Set the Right Sibling Offset */
	new_leaf->setOffset(DEFAULT_LEAF_ORDER - 1, leaf->getOffset(DEFAULT_LEAF_ORDER - 1));
	leaf->setOffset(DEFAULT_LEAF_ORDER - 1, new_leaf_offset);

    /* Clear unused space */
    for (i = leaf->getNumOfKeys(); i < DEFAULT_LEAF_ORDER - 1; i++) {
		leaf->setKey(i, 0);
		leaf->setValues(i, nullptr);
    }
    for (i = new_leaf->getNumOfKeys(); i < DEFAULT_LEAF_ORDER - 1; i++) {
		new_leaf->setKey(i, 0);
		new_leaf->setValues(i, nullptr);
    }

    /* Set the parent offset */
	new_leaf->setParentOffset(leaf->getParentOffset());

    /* Take the first element of new leaf as a push-up key. */
    new_key = new_leaf->getKey(0);

    BufferManager::put_frame(buf_leaf);
    BufferManager::put_frame(buf_new_leaf);

    return insert_into_parent(table_id, root_offset, leaf_offset, new_key, new_leaf_offset);
}

/*
 * Insert a key into an internal page to save a record.
 * To save the key, it has to determine whether to split and whether to push a key.
 */
offset_t insert_into_parent(int table_id, offset_t root_offset, offset_t left_offset, uint64_t key, offset_t right_offset) {
    int left_index = 0, num_keys;
    offset_t parent_offset;

    BufferBlock *buf_left, *buf_parent;
    Page *left, *parent;
    
    buf_left = BufferManager::get_frame(table_id, PGNUM(left_offset));
    assert(buf_left != NULL);

    left = BufferManager::get_page(buf_left, false);

    parent_offset = left->getParentOffset();

    BufferManager::put_frame(buf_left);

    /* Case: new root. */

    if (parent_offset == HEADER_PAGE_OFFSET)
        return insert_into_new_root(table_id, left_offset, key, right_offset);

    /* Case: leaf or node. (Remainder of function body.)
     */

    /* Find the parent's pointer to the left node.
     */

    left_index = get_left_index(table_id, parent_offset, left_offset);

    /* Simple case: the new key fits into the node.
     */
    buf_parent = BufferManager::get_frame(table_id, PGNUM(parent_offset));
    assert(buf_parent != NULL);
    parent = BufferManager::get_page(buf_parent, false);

    num_keys = parent->getNumOfKeys();

    BufferManager::put_frame(buf_parent);

    if (num_keys < DEFAULT_INTERNAL_ORDER - 1)
        return insert_into_node(table_id, root_offset, parent_offset, left_index, key, right_offset);

    /* Harder case:  split a node in order to preserve the B+ tree properties.
     */

    return insert_into_node_after_splitting(table_id, root_offset, parent_offset, left_index, key, right_offset);
}

/*
 * Insert a key into a internal page which has a room to save a key.
 */
offset_t insert_into_node(int table_id, offset_t root_offset, offset_t parent_offset, int left_index, uint64_t key, offset_t right_offset) {
    BufferBlock* buf_parent;
    Page* parent;

    buf_parent = BufferManager::get_frame(table_id, PGNUM(parent_offset));
    assert(buf_parent != NULL);
    parent = BufferManager::get_page(buf_parent, true);

    int i, num_keys = parent->getNumOfKeys();

    for (i = num_keys; i > left_index; i--) {
		parent->setOffset(i + 1, parent->getOffset(i));
		parent->setKey(i, parent->getKey(i - 1));
    }
	parent->setOffset(left_index + 1, right_offset);
	parent->setKey(left_index, key);
	parent->setNumOfKeys(parent->getNumOfKeys() + 1);

    BufferManager::put_frame(buf_parent);
    return root_offset;
}

/*
 * Insert a key into a internal page which has no room to save more keys.
 * Therefore, first split the internal page and call insert_into_parent() for pushing up the key recursively.
 */
offset_t insert_into_node_after_splitting(int table_id, offset_t root_offset, offset_t parent_offset, int left_index, uint64_t key, offset_t right_offset) {
    int i, j, split, num_keys;
    offset_t new_node_offset, child_offset;
    record_key_t temp_keys[DEFAULT_INTERNAL_ORDER], pushup_key;
    offset_t temp_pointers[DEFAULT_INTERNAL_ORDER + 1];

    /* First create a temporary set of keys and pointers
     * to hold everything in order, including
     * the new key and pointer, inserted in their
     * correct places.
     * Then create a new node and copy half of the
     * keys and pointers to the old node and
     * the other half to the new.
     */
    BufferBlock *buf_parent, *buf_new_node, *buf_child;
    Page *parent, *new_node, *child;

    buf_parent = BufferManager::get_frame(table_id, PGNUM(parent_offset));
    assert(buf_parent != NULL);
    parent = BufferManager::get_page(buf_parent, true);

    num_keys = parent->getNumOfKeys();

    for (i = 0, j = 0; i < num_keys + 1; i++, j++) {
        if (j == left_index + 1) j++;
        temp_pointers[j] = parent->getOffset(i);
    }

    for (i = 0, j = 0; i < num_keys; i++, j++) {
        if (j == left_index) j++;
        temp_keys[j] = parent->getKey(i);
    }

    temp_pointers[left_index + 1] = right_offset;
    temp_keys[left_index] = key;

    /* Create the new node and copy half the keys and pointers to the old and half to the new. */
    split = cut(DEFAULT_INTERNAL_ORDER);
    new_node_offset = make_internal(table_id);

    buf_new_node = BufferManager::get_frame(table_id, PGNUM(new_node_offset));
    assert(buf_new_node != NULL);
    new_node = BufferManager::get_page(buf_new_node, true);

    /* Left internal node */
    for (i = 0, num_keys = 0; i < split - 1; i++) {
		parent->setOffset(i, temp_pointers[i]);
		parent->setKey(i, temp_keys[i]);
		parent->setNumOfKeys(++num_keys);
    }
	parent->setOffset(split - 1, temp_pointers[split - 1]); // Last Offset

    pushup_key = temp_keys[split - 1];

    /* Right internal node */
    for (++i, j = 0, num_keys = 0; i < DEFAULT_INTERNAL_ORDER; i++, j++) {
        new_node->setOffset(j, temp_pointers[i]);
		new_node->setKey(j, temp_keys[i]);
		new_node->setNumOfKeys(++num_keys);
    }
	new_node->setOffset(j, temp_pointers[i]);  // Last Offset

    /* Set the parent offset of new_node same as the left node. */
	new_node->setParentOffset(parent->getParentOffset());
    for (i = 0, num_keys = new_node->getNumOfKeys(); i <= num_keys; i++) {
        child_offset = new_node->getOffset(i);
        /* Set the parent offset of child nodes new_node_offset. */
        buf_child = BufferManager::get_frame(table_id, PGNUM(child_offset));
        assert(buf_child != NULL);
        child = BufferManager::get_page(buf_child, true);

		child->setParentOffset(new_node_offset);

        BufferManager::put_frame(buf_child);
    }

    BufferManager::put_frame(buf_new_node);
    BufferManager::put_frame(buf_parent);

    /* Insert a new key into the parent of the two
     * nodes resulting from the split, with
     * the old node to the left and the new to the right.
     */

    return insert_into_parent(table_id, root_offset, parent_offset, pushup_key, new_node_offset);
}

/*
 * Make a new root page for insertion of the pushup key and insert it into the root page.
 */
offset_t insert_into_new_root( int table_id, offset_t left_offset, uint64_t key, offset_t right_offset) {
    BufferBlock *buf_root, *buf_left, *buf_right;
    Page *root, *left, *right;
    offset_t root_offset = make_internal(table_id);
    buf_root = BufferManager::get_frame(table_id, PGNUM(root_offset));
    buf_left = BufferManager::get_frame(table_id, PGNUM(left_offset));
    buf_right = BufferManager::get_frame(table_id, PGNUM(right_offset));
    assert(buf_root != NULL);
    assert(buf_left != NULL);
    assert(buf_right != NULL);
    root = BufferManager::get_page(buf_root, true);
    left = BufferManager::get_page(buf_left, true);
    right = BufferManager::get_page(buf_right, true);

	root->setKey(0, key);
	root->setOffset(0, left_offset);
	root->setOffset(1, right_offset);
	root->setNumOfKeys(1);
	root->setParentOffset(HEADER_PAGE_OFFSET);
    left->setParentOffset(root_offset);
    right->setParentOffset(root_offset);

    BufferManager::put_frame(buf_root);
    BufferManager::put_frame(buf_left);
    BufferManager::put_frame(buf_right);
    return root_offset;
}

/*
 * If there is no tree, make a new tree.
 */
offset_t start_new_tree(int table_id, const record_t* record) {
    offset_t root_offset = make_leaf(table_id);
    BufferBlock *buf;
    Page *root;

    buf = BufferManager::get_frame(table_id, PGNUM(root_offset));
    assert(buf != NULL);
    root = BufferManager::get_page(buf, true);

    root->setKey(0, record->key);
	root->setValues(0, record->values, getNumOfCols(table_id));
	root->setNumOfKeys(1);

    BufferManager::put_frame(buf);

    return root_offset;
}