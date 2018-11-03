#include "insert.h"

#include <string.h>
#include <stdlib.h>

/*
 * Make an intenal page and return its offset.
 */
offset_t make_internal( void ) {
    page_t buf;

    // Allocate one page from the free page list.
    offset_t new_page_offset = file_alloc_page();

    file_read_page(PGNUM(new_page_offset), &buf);

    CLEAR(buf);
    setInternal(&buf);

    file_write_page(PGNUM(new_page_offset), &buf);

    return new_page_offset;
}

/*
 * Make a leaf page and return its offset.
 */
offset_t make_leaf( void ) {
    page_t buf;

    // Allocate one page from the free page list.
    offset_t new_page_offset = file_alloc_page();

    file_read_page(PGNUM(new_page_offset), &buf);

    CLEAR(buf);
    setLeaf(&buf);

    file_write_page(PGNUM(new_page_offset), &buf);

    return new_page_offset;
}

/*
 * Get the index of a left page in terms of a parent page.
 */
int get_left_index( offset_t parent_offset, offset_t left_offset ) {
    page_t parent;

    file_read_page(PGNUM(parent_offset), &parent);

    int num_keys = getNumOfKeys(&parent);
    int left_index = 0;
    while (left_index <= num_keys && getOffset(&parent, left_index) != left_offset)
        left_index++;

    return left_index;
}

/*
 * Insert a record into a leaf page which has a room to save a record.
 */
offset_t insert_into_leaf( offset_t leaf, const record_t * record ) {
    char buf_value[120];
    int i, insertion_point, num_keys;
    page_t buf;

    file_read_page(PGNUM(leaf), &buf);

    /* Load the metatdata. */
    num_keys = getNumOfKeys(&buf);

    /* Find the insertion point by binary search. */
    if (record->key <  getKey(&buf, 0))
        insertion_point = 0;
    else {
        int index = binaryRangeSearch(&buf, record->key);
        if (index == INVALID_KEY)
            return INVALID_KEY;

        insertion_point = index + 1;
    }

    /* Shifting */
    for (i = num_keys; i > insertion_point; i--) {
        setKey(&buf, i, getKey(&buf, i - 1));
        getValue(&buf, i - 1, buf_value);
        setValue(&buf, i, buf_value);
    }
    /* Insertion */
    setKey(&buf, insertion_point, record->key);
    setValue(&buf, insertion_point, record->value);

    /* Set Metadata */
    setNumOfKeys(&buf, getNumOfKeys(&buf) + 1);

    file_write_page(PGNUM(leaf), &buf);
    return leaf;
}

/*
 * Insert a record into a leaf page which has no room to save more records.
 * Therefore, first split the leaf page and call insert_into_parent() for pushing up the key.
 */
offset_t insert_into_leaf_after_splitting( offset_t root_offset, offset_t leaf_offset, const record_t * record) {
    page_t leaf, new_leaf;
    offset_t new_leaf_offset;
    keynum_t temp_keys[DEFAULT_LEAF_ORDER];
    char temp_values[DEFAULT_LEAF_ORDER][120];
    int insertion_index, split, new_key, i, j, num_keys;

    new_leaf_offset = make_leaf();

    file_read_page(PGNUM(leaf_offset), &leaf);
    file_read_page(PGNUM(new_leaf_offset), &new_leaf);

    /* Find the Insertion Point */
    if (record->key < getKey(&leaf, 0))
        insertion_index = 0;
    else
        insertion_index = binaryRangeSearch(&leaf, record->key) + 1;

    /* Move to temporary space. */
    for (i = 0, j = 0, num_keys = getNumOfKeys(&leaf); i < num_keys; i++, j++) {
        if (j == insertion_index) j++;
        temp_keys[j] = getKey(&leaf, i);
        getValue(&leaf, i, temp_values[j]);
    }

    /* Insertion */
    temp_keys[insertion_index] = record->key;
    strncpy(temp_values[insertion_index], record->value, 120);

    /* Split */
    split = cut(DEFAULT_LEAF_ORDER - 1);

    /* Distribute the records into two split leaf nodes. */
    for (i = 0, num_keys = 0; i < split; i++) {
        setValue(&leaf, i, temp_values[i]);
        setKey(&leaf, i, temp_keys[i]);
        setNumOfKeys(&leaf, ++num_keys);
    }

    for (i = split, j = 0, num_keys = 0; i < DEFAULT_LEAF_ORDER; i++, j++) {
        setValue(&new_leaf, j, temp_values[i]);
        setKey(&new_leaf, j, temp_keys[i]);
        setNumOfKeys(&new_leaf, ++num_keys);
    }

    /* Set the Right Sibling Offset */
    setOffset(&new_leaf, DEFAULT_LEAF_ORDER - 1, getOffset(&leaf, DEFAULT_LEAF_ORDER - 1));
    setOffset(&leaf, DEFAULT_LEAF_ORDER - 1, new_leaf_offset);

    /* Clear unused space */
    for (i = getNumOfKeys(&leaf); i < DEFAULT_LEAF_ORDER - 1; i++) {
        setKey(&leaf, i, 0);
        setValue(&leaf, i, "");
    }
    for (i = getNumOfKeys(&new_leaf); i < DEFAULT_LEAF_ORDER - 1; i++) {
        setKey(&new_leaf, i, 0);
        setValue(&new_leaf, i, "");
    }

    /* Set the parent offset */
    setParentOffset(&new_leaf, getParentOffset(&leaf));

    /* Take the first element of new leaf as a push-up key. */
    new_key = getKey(&new_leaf, 0);

    file_write_page(PGNUM(leaf_offset), &leaf);
    file_write_page(PGNUM(new_leaf_offset), &new_leaf);

    return insert_into_parent(root_offset, leaf_offset, new_key, new_leaf_offset);
}

/*
 * Insert a key into an internal page to save a record.
 * To save the key, it has to determine whether to split and whether to push a key.
 */
offset_t insert_into_parent( offset_t root_offset, offset_t left_offset, uint64_t key, offset_t right_offset ) {
    int left_index = 0, num_keys;
    offset_t parent_offset;
    page_t left, parent;

    file_read_page(PGNUM(left_offset), &left);

    parent_offset = getParentOffset(&left);

    /* Case: new root. */

    if (parent_offset == HEADER_PAGE_OFFSET)
        return insert_into_new_root(left_offset, key, right_offset);

    /* Case: leaf or node. (Remainder of function body.)
     */

    /* Find the parent's pointer to the left node.
     */

    left_index = get_left_index(parent_offset, left_offset);

    /* Simple case: the new key fits into the node.
     */

    file_read_page(PGNUM(parent_offset), &parent);
    num_keys = getNumOfKeys(&parent);
    if (num_keys < DEFAULT_INTERNAL_ORDER - 1)
        return insert_into_node(root_offset, parent_offset, left_index, key, right_offset);

    /* Harder case:  split a node in order to preserve the B+ tree properties.
     */

    return insert_into_node_after_splitting(root_offset, parent_offset, left_index, key, right_offset);
}

/*
 * Insert a key into a internal page which has a room to save a key.
 */
offset_t insert_into_node( offset_t root_offset, offset_t parent_offset, int left_index, uint64_t key, offset_t right_offset) {
    page_t parent;
    file_read_page(PGNUM(parent_offset), &parent);

    int i, num_keys = getNumOfKeys(&parent);

    for (i = num_keys; i > left_index; i--) {
        setOffset(&parent, i + 1, getOffset(&parent, i));
        setKey(&parent, i, getKey(&parent, i - 1));
    }
    setOffset(&parent, left_index + 1, right_offset);
    setKey(&parent, left_index, key);
    setNumOfKeys(&parent, getNumOfKeys(&parent) + 1);

    file_write_page(PGNUM(parent_offset), &parent);
    return root_offset;
}

/*
 * Insert a key into a internal page which has no room to save more keys.
 * Therefore, first split the internal page and call insert_into_parent() for pushing up the key recursively.
 */
offset_t insert_into_node_after_splitting( offset_t root_offset, offset_t parent_offset, int left_index, uint64_t key, offset_t right_offset) {
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
    page_t parent, new_node, child;

    file_read_page(PGNUM(parent_offset), &parent);
    num_keys = getNumOfKeys(&parent);

    for (i = 0, j = 0; i < num_keys + 1; i++, j++) {
        if (j == left_index + 1) j++;
        temp_pointers[j] = getOffset(&parent, i);
    }

    for (i = 0, j = 0; i < num_keys; i++, j++) {
        if (j == left_index) j++;
        temp_keys[j] = getKey(&parent, i);
    }

    temp_pointers[left_index + 1] = right_offset;
    temp_keys[left_index] = key;

    /* Create the new node and copy half the keys and pointers to the old and half to the new. */
    split = cut(DEFAULT_INTERNAL_ORDER);
    new_node_offset = make_internal();
    file_read_page(PGNUM(new_node_offset), &new_node);

    /* Left internal node */
    for (i = 0, num_keys = 0; i < split - 1; i++) {
        setOffset(&parent, i, temp_pointers[i]);
        setKey(&parent, i, temp_keys[i]);
        setNumOfKeys(&parent, ++num_keys);
    }
    setOffset(&parent, split - 1, temp_pointers[split - 1]); // Last Offset

    pushup_key = temp_keys[split - 1];

    /* Right internal node */
    for (++i, j = 0, num_keys = 0; i < DEFAULT_INTERNAL_ORDER; i++, j++) {
        setOffset(&new_node, j, temp_pointers[i]);
        setKey(&new_node, j, temp_keys[i]);
        setNumOfKeys(&new_node, ++num_keys);
    }
    setOffset(&new_node, j, temp_pointers[i]);  // Last Offset

    /* Set the parent offset of new_node same as the left node. */
    setParentOffset(&new_node, getParentOffset(&parent));
    for (i = 0, num_keys = getNumOfKeys(&new_node); i <= num_keys; i++) {
        child_offset = getOffset(&new_node, i);
        /* Set the parent offset of child nodes new_node_offset. */
        file_read_page(PGNUM(child_offset), &child);
        setParentOffset(&child, new_node_offset);
        file_write_page(PGNUM(child_offset), &child);
    }

    file_write_page(PGNUM(new_node_offset), &new_node);
    file_write_page(PGNUM(parent_offset), &parent);

    /* Insert a new key into the parent of the two
     * nodes resulting from the split, with
     * the old node to the left and the new to the right.
     */

    return insert_into_parent(root_offset, parent_offset, pushup_key, new_node_offset);
}

/*
 * Make a new root page for insertion of the pushup key and insert it into the root page.
 */
offset_t insert_into_new_root( offset_t left_offset, uint64_t key, offset_t right_offset ) {
    page_t root, left, right;
    offset_t root_offset = make_internal();
    file_read_page(PGNUM(root_offset), &root);
    file_read_page(PGNUM(left_offset), &left);
    file_read_page(PGNUM(right_offset), &right);

    setKey(&root, 0, key);
    setOffset(&root, 0, left_offset);
    setOffset(&root, 1, right_offset);
    setNumOfKeys(&root, 1);
    setParentOffset(&root, HEADER_PAGE_OFFSET);
    setParentOffset(&left, root_offset);
    setParentOffset(&right, root_offset);

    file_write_page(PGNUM(root_offset), &root);
    file_write_page(PGNUM(left_offset), &left);
    file_write_page(PGNUM(right_offset), &right);

    return root_offset;
}

/*
 * If there is no tree, make a new tree.
 */
offset_t start_new_tree( const record_t* record ) {
    offset_t root = make_leaf();
    page_t buf;

    file_read_page(PGNUM(root), &buf);

    setKey(&buf, 0, record->key);
    setValue(&buf, 0, record->value);
    setNumOfKeys(&buf, 1);

    file_write_page(PGNUM(root), &buf);

    return root;
}

/* Master insertion function.
* Inserts a key and an associated value into
* the B+ tree, causing the tree to be adjusted
* however necessary to maintain the B+ tree
* properties.
*/

offset_t insert_record( offset_t root, const record_t * record ) {
    record_t * pointer;
    page_t buf;

    /* The current implementation ignores duplicates.
     */

    // Duplicated
    if ((pointer = find_record(root, record->key)) != NULL) {
        free(pointer);
        return root;
    }

    /* Case: the tree does not exist yet.
     * Start a new tree.
     */

    if (root == HEADER_PAGE_OFFSET)
        return start_new_tree(record);

    offset_t leaf = find_leaf(root, record->key);

    if(leaf == HEADER_PAGE_OFFSET)
        return root;
    
    file_read_page(PGNUM(leaf), &buf);

    /* Case: the tree already exists.
     * (Rest of function body.)
     */

    /* Case: leaf has room for key and pointer.
     */

    if (getNumOfKeys(&buf) < DEFAULT_LEAF_ORDER - 1) {
        leaf = insert_into_leaf(leaf, record);
        return root;
    }

    /* Case:  leaf must be split.
     */

    return insert_into_leaf_after_splitting(root, leaf, record);
}