#include "index_and_file_manager.h"

#include "utils.h"
#include "macros.h"
#include "buffer_manager.h"
#include "page_access_manager.h"

#include <stdlib.h>
#include <string.h>

/*
 * Make an intenal page and return its offset.
 */
offset_t make_internal(int table_id) {
    // Allocate one page from the free page list.
    offset_t new_page_offset = file_alloc_page(table_id);
    buffer_frame_t *buf = buf_get_frame(table_id, PGNUM(new_page_offset));
    page_t *internal_node = buf_get_page(buf, true);

    CLEAR(internal_node);
    setInternal(internal_node);

    buf_put_frame(buf);
    return new_page_offset;
}

/*
 * Make a leaf page and return its offset.
 */
offset_t make_leaf( int table_id ) {
    // Allocate one page from the free page list.
    offset_t new_page_offset = file_alloc_page(table_id);
    buffer_frame_t *buf = buf_get_frame(table_id, PGNUM(new_page_offset));
    page_t *leaf_node = buf_get_page(buf, true);

    CLEAR(leaf_node);
    setLeaf(leaf_node);

    buf_put_frame(buf);
    return new_page_offset;
}

/*
 * Get the index of a left page in terms of a parent page.
 */
int get_left_index(int table_id, offset_t parent_offset, offset_t left_offset) {
    buffer_frame_t* buf = NULL;
    buf = buf_get_frame(table_id, PGNUM(parent_offset));
    assert(buf != NULL);

    page_t* parent = buf_get_page(buf, false);

    int num_keys = getNumOfKeys(parent);
    int left_index = 0;
    while (left_index <= num_keys && getOffset(parent, left_index) != left_offset)
        left_index++;

    buf_put_frame(buf);
    return left_index;
}

/*
 * Insert a record into a leaf page which has a room to save a record.
 */
offset_t insert_into_leaf(int table_id, offset_t leaf_offset, const record_t* record) {
    char buf_value[DEFAULT_RECORD_SIZE];
    int i, insertion_point, num_keys;

    buffer_frame_t* buf = NULL;
    buf = buf_get_frame(table_id, PGNUM(leaf_offset));
    assert(buf != NULL);

    page_t* leaf = buf_get_page(buf, true);

    /* Load the metatdata. */
    num_keys = getNumOfKeys(leaf);

    /* Find the insertion point by binary search. */
    if (record->key <  getKey(leaf, 0))
        insertion_point = 0;
    else {
        int index = binaryRangeSearch(leaf, record->key);
        if (index == INVALID_KEY){
            buf_put_frame(buf);
            return INVALID_KEY;
        }

        insertion_point = index + 1;
    }

    /* Shifting */
    for (i = num_keys; i > insertion_point; i--) {
        setKey(leaf, i, getKey(leaf, i - 1));
        getValue(leaf, i - 1, buf_value);
        setValue(leaf, i, buf_value);
    }

    /* Insertion */
    setKey(leaf, insertion_point, record->key);
    setValue(leaf, insertion_point, record->value);

    /* Set Metadata */
    setNumOfKeys(leaf, getNumOfKeys(leaf) + 1);

    /* Set it dirty. */
    buf_put_frame(buf);
    return leaf_offset;
}

/*
 * Insert a record into a leaf page which has no room to save more records.
 * Therefore, first split the leaf page and call insert_into_parent() for pushing up the key.
 */
offset_t insert_into_leaf_after_splitting(int table_id, offset_t root_offset, offset_t leaf_offset, const record_t* record) {
    buffer_frame_t *buf_leaf, *buf_new_leaf;
    page_t *leaf, *new_leaf;
    offset_t new_leaf_offset;
    keynum_t temp_keys[DEFAULT_LEAF_ORDER];
    char temp_values[DEFAULT_LEAF_ORDER][120];
    int insertion_index, split, new_key, i, j, num_keys;

    new_leaf_offset = make_leaf(table_id);

    buf_leaf = buf_get_frame(table_id, PGNUM(leaf_offset));
    buf_new_leaf = buf_get_frame(table_id, PGNUM(new_leaf_offset));
    assert(buf_leaf != NULL);
    assert(buf_new_leaf != NULL);

    leaf = buf_get_page(buf_leaf, true);
    new_leaf = buf_get_page(buf_new_leaf, true);

    /* Find the Insertion Point */
    if (record->key < getKey(leaf, 0))
        insertion_index = 0;
    else
        insertion_index = binaryRangeSearch(leaf, record->key) + 1;

    /* Move to temporary space. */
    for (i = 0, j = 0, num_keys = getNumOfKeys(leaf); i < num_keys; i++, j++) {
        if (j == insertion_index) j++;
        temp_keys[j] = getKey(leaf, i);
        getValue(leaf, i, temp_values[j]);
    }

    /* Insertion */
    temp_keys[insertion_index] = record->key;
    strncpy(temp_values[insertion_index], record->value, 120);

    /* Split */
    split = cut(DEFAULT_LEAF_ORDER - 1);

    /* Distribute the records into two split leaf nodes. */
    for (i = 0, num_keys = 0; i < split; i++) {
        setValue(leaf, i, temp_values[i]);
        setKey(leaf, i, temp_keys[i]);
        setNumOfKeys(leaf, ++num_keys);
    }

    for (i = split, j = 0, num_keys = 0; i < DEFAULT_LEAF_ORDER; i++, j++) {
        setValue(new_leaf, j, temp_values[i]);
        setKey(new_leaf, j, temp_keys[i]);
        setNumOfKeys(new_leaf, ++num_keys);
    }

    /* Set the Right Sibling Offset */
    setOffset(new_leaf, DEFAULT_LEAF_ORDER - 1, getOffset(leaf, DEFAULT_LEAF_ORDER - 1));
    setOffset(leaf, DEFAULT_LEAF_ORDER - 1, new_leaf_offset);

    /* Clear unused space */
    for (i = getNumOfKeys(leaf); i < DEFAULT_LEAF_ORDER - 1; i++) {
        setKey(leaf, i, 0);
        setValue(leaf, i, "");
    }
    for (i = getNumOfKeys(new_leaf); i < DEFAULT_LEAF_ORDER - 1; i++) {
        setKey(new_leaf, i, 0);
        setValue(new_leaf, i, "");
    }

    /* Set the parent offset */
    setParentOffset(new_leaf, getParentOffset(leaf));

    /* Take the first element of new leaf as a push-up key. */
    new_key = getKey(new_leaf, 0);

    buf_put_frame(buf_leaf);
    buf_put_frame(buf_new_leaf);

    return insert_into_parent(table_id, root_offset, leaf_offset, new_key, new_leaf_offset);
}

/*
 * Insert a key into an internal page to save a record.
 * To save the key, it has to determine whether to split and whether to push a key.
 */
offset_t insert_into_parent(int table_id, offset_t root_offset, offset_t left_offset, uint64_t key, offset_t right_offset) {
    int left_index = 0, num_keys;
    offset_t parent_offset;

    buffer_frame_t *buf_left, *buf_parent;
    page_t *left, *parent;
    
    buf_left = buf_get_frame(table_id, PGNUM(left_offset));
    assert(buf_left != NULL);

    left = buf_get_page(buf_left, false);

    parent_offset = getParentOffset(left);

    buf_put_frame(buf_left);

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
    buf_parent = buf_get_frame(table_id, PGNUM(parent_offset));
    assert(buf_parent != NULL);
    parent = buf_get_page(buf_parent, false);

    num_keys = getNumOfKeys(parent);

    buf_put_frame(buf_parent);

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
    buffer_frame_t* buf_parent;
    page_t* parent;

    buf_parent = buf_get_frame(table_id, PGNUM(parent_offset));
    assert(buf_parent != NULL);
    parent = buf_get_page(buf_parent, true);

    int i, num_keys = getNumOfKeys(parent);

    for (i = num_keys; i > left_index; i--) {
        setOffset(parent, i + 1, getOffset(parent, i));
        setKey(parent, i, getKey(parent, i - 1));
    }
    setOffset(parent, left_index + 1, right_offset);
    setKey(parent, left_index, key);
    setNumOfKeys(parent, getNumOfKeys(parent) + 1);

    buf_put_frame(buf_parent);
    return root_offset;
}

/*
 * Insert a key into a internal page which has no room to save more keys.
 * Therefore, first split the internal page and call insert_into_parent() for pushing up the key recursively.
 */
offset_t insert_into_node_after_splitting(int table_id, offset_t root_offset, offset_t parent_offset, int left_index, uint64_t key, offset_t right_offset) {
    int i, j, split, num_keys;
    offset_t new_node_offset, child_offset;
    keynum_t temp_keys[DEFAULT_INTERNAL_ORDER], pushup_key;
    offset_t temp_pointers[DEFAULT_INTERNAL_ORDER + 1];

    /* First create a temporary set of keys and pointers
     * to hold everything in order, including
     * the new key and pointer, inserted in their
     * correct places.
     * Then create a new node and copy half of the
     * keys and pointers to the old node and
     * the other half to the new.
     */
    buffer_frame_t *buf_parent, *buf_new_node, *buf_child;
    page_t *parent, *new_node, *child;

    buf_parent = buf_get_frame(table_id, PGNUM(parent_offset));
    assert(buf_parent != NULL);
    parent = buf_get_page(buf_parent, true);

    num_keys = getNumOfKeys(parent);

    for (i = 0, j = 0; i < num_keys + 1; i++, j++) {
        if (j == left_index + 1) j++;
        temp_pointers[j] = getOffset(parent, i);
    }

    for (i = 0, j = 0; i < num_keys; i++, j++) {
        if (j == left_index) j++;
        temp_keys[j] = getKey(parent, i);
    }

    temp_pointers[left_index + 1] = right_offset;
    temp_keys[left_index] = key;

    /* Create the new node and copy half the keys and pointers to the old and half to the new. */
    split = cut(DEFAULT_INTERNAL_ORDER);
    new_node_offset = make_internal(table_id);

    buf_new_node = buf_get_frame(table_id, PGNUM(new_node_offset));
    assert(buf_new_node != NULL);
    new_node = buf_get_page(buf_new_node, true);

    /* Left internal node */
    for (i = 0, num_keys = 0; i < split - 1; i++) {
        setOffset(parent, i, temp_pointers[i]);
        setKey(parent, i, temp_keys[i]);
        setNumOfKeys(parent, ++num_keys);
    }
    setOffset(parent, split - 1, temp_pointers[split - 1]); // Last Offset

    pushup_key = temp_keys[split - 1];

    /* Right internal node */
    for (++i, j = 0, num_keys = 0; i < DEFAULT_INTERNAL_ORDER; i++, j++) {
        setOffset(new_node, j, temp_pointers[i]);
        setKey(new_node, j, temp_keys[i]);
        setNumOfKeys(new_node, ++num_keys);
    }
    setOffset(new_node, j, temp_pointers[i]);  // Last Offset

    /* Set the parent offset of new_node same as the left node. */
    setParentOffset(new_node, getParentOffset(parent));
    for (i = 0, num_keys = getNumOfKeys(new_node); i <= num_keys; i++) {
        child_offset = getOffset(new_node, i);
        /* Set the parent offset of child nodes new_node_offset. */
        buf_child = buf_get_frame(table_id, PGNUM(child_offset));
        assert(buf_child != NULL);
        child = buf_get_page(buf_child, true);

        setParentOffset(child, new_node_offset);

        buf_put_frame(buf_child);
    }

    buf_put_frame(buf_new_node);
    buf_put_frame(buf_parent);

    /* Insert a new key into the parent of the two
     * nodes resulting from the split, with
     * the old node to the left and the new to the right.
     */

    return insert_into_parent(table_id, root_offset, parent_offset, pushup_key, new_node_offset);
}

/*
 * Make a new root page for insertion of the pushup key and insert it into the root page.
 */
offset_t insert_into_new_root( int table_id, offset_t left_offset, uint64_t key, offset_t right_offset ) {
    buffer_frame_t *buf_root, *buf_left, *buf_right;
    page_t *root, *left, *right;
    offset_t root_offset = make_internal(table_id);
    buf_root = buf_get_frame(table_id, PGNUM(root_offset));
    buf_left = buf_get_frame(table_id, PGNUM(left_offset));
    buf_right = buf_get_frame(table_id, PGNUM(right_offset));
    assert(buf_root != NULL);
    assert(buf_left != NULL);
    assert(buf_right != NULL);
    root = buf_get_page(buf_root, true);
    left = buf_get_page(buf_left, true);
    right = buf_get_page(buf_right, true);

    setKey(root, 0, key);
    setOffset(root, 0, left_offset);
    setOffset(root, 1, right_offset);
    setNumOfKeys(root, 1);
    setParentOffset(root, HEADER_PAGE_OFFSET);
    setParentOffset(left, root_offset);
    setParentOffset(right, root_offset);

    buf_put_frame(buf_root);
    buf_put_frame(buf_left);
    buf_put_frame(buf_right);
    return root_offset;
}

/*
 * If there is no tree, make a new tree.
 */
offset_t start_new_tree(int table_id, const record_t* record) {
    offset_t root_offset = make_leaf(table_id);
    buffer_frame_t *buf;
    page_t *root;

    buf = buf_get_frame(table_id, PGNUM(root_offset));
    assert(buf != NULL);
    root = buf_get_page(buf, true);

    setKey(root, 0, record->key);
    setValue(root, 0, record->value);
    setNumOfKeys(root, 1);

    buf_put_frame(buf);

    return root_offset;
}

/* Master insertion function.
* Inserts a key and an associated value into
* the B+ tree, causing the tree to be adjusted
* however necessary to maintain the B+ tree
* properties.
*/

offset_t insert_record(int table_id, offset_t root_offset, const record_t* record) {
    record_t * pointer;
    buffer_frame_t *buf_leaf;
    page_t *leaf;

    /* The current implementation ignores duplicates.
     */

    // Duplicated
    if ((pointer = find_record(table_id, root_offset, record->key)) != NULL) {
        free(pointer);
        return root_offset;
    }

    /* Case: the tree does not exist yet.
     * Start a new tree.
     */

    if (root_offset == HEADER_PAGE_OFFSET)
        return start_new_tree(table_id, record);

    offset_t leaf_offset = find_leaf(table_id, root_offset, record->key);

    if(leaf == HEADER_PAGE_OFFSET)
        return root_offset;
    
    /* Case: the tree already exists.
     * (Rest of function body.)
     */

    /* Case: leaf has room for key and pointer.
     */

    buf_leaf = buf_get_frame(table_id, PGNUM(leaf_offset));
    assert(buf_leaf != NULL);
    leaf = buf_get_page(buf_leaf, false);
    int num_keys = getNumOfKeys(leaf);
    buf_put_frame(buf_leaf);

    if (num_keys < DEFAULT_LEAF_ORDER - 1) {
        leaf_offset = insert_into_leaf(table_id, leaf_offset, record);
        return root_offset;
    }

    /* Case:  leaf must be split.
     */

    return insert_into_leaf_after_splitting(table_id, root_offset, leaf_offset, record);
}