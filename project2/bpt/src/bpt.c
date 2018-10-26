#include "bpt.h"

/*
 *  Find the record containing input ‘key’.
 *  If found matching ‘key’, return matched ‘value’ string. Otherwise, return NULL.
 */
char * find (int64_t key) {
    record_t* record;

    file_read_page(HEADER_PAGE_NUM, &header);

    if ((record = find_record(getRootPageOffset(&header), key)) == NULL)
        return NULL;

    char * ret = malloc(120);
    strncpy(ret, record->value, 120);
    free(record);
    return ret;
}

/*
 *  Insert input ‘key/value’ (record) to data file at the right place.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int insert (int64_t key, char * value) {
    record_t record;

    // Set the data
    record.key = key;
    strncpy(record.value, value, 120);

    file_read_page(HEADER_PAGE_NUM, &header);

    // Insert the record
    offset_t root_offset = insert_record(getRootPageOffset(&header), &record);

    // Update the root offset
    file_read_page(HEADER_PAGE_NUM, &header);
    setRootPageOffset(&header, root_offset);
    file_write_page(HEADER_PAGE_NUM, &header);

    return root_offset == KEY_EXIST;
}

/*
 *  Allocate an on-disk page from the free page list
 */
pagenum_t file_alloc_page() {

    /*
     * Only if a file doesn't have enough free pages,
     * add free pages into the free page list
     * with an amount of DEFAULT_SIZE_OF_FREE_PAGES.
     */

    /* Update the cached header page. */
    file_read_page(HEADER_PAGE_NUM, &header);

    page_t buf;

    if (getFreePageOffset(&header) == HEADER_PAGE_OFFSET) {
        CLEAR(buf);
        int i;
        pagenum_t next_free_page_num = getNumOfPages(&header);
        setFreePageOffset(&header, OFFSET(next_free_page_num));
        for (i = 1; i <= DEFAULT_SIZE_OF_FREE_PAGES; ++i, ++next_free_page_num) {
            setNextFreePageOffset(&buf, i != DEFAULT_SIZE_OF_FREE_PAGES ? OFFSET(next_free_page_num + 1) : HEADER_PAGE_OFFSET);
            file_write_page(next_free_page_num, &buf);
        }
        // Set Number of Pages
        setNumOfPages(&header, getNumOfPages(&header) + DEFAULT_SIZE_OF_FREE_PAGES);
        // Sync the cached header page with the file.
        file_write_page(HEADER_PAGE_NUM, &header);
    }
    // Get a free page offset from header page.
    offset_t free_page_offset = getFreePageOffset(&header);
    // Read the free page.
    file_read_page(PGNUM(free_page_offset), &buf);
    // Get a next free page offset from the free page.
    setFreePageOffset(&header, getNextFreePageOffset(&buf));
    // Wrtie the header page and sync the cached header page with the file.
    file_write_page(HEADER_PAGE_NUM, &header);

    return free_page_offset;
}

/*
 *  Free an on-disk page to the free page list
 */
void file_free_page(pagenum_t pagenum) {
    page_t buf;

    // Read the given page and header page.
    file_read_page(pagenum, &buf);
    file_read_page(HEADER_PAGE_NUM, &header);

    // Clear
    CLEAR(buf);

    // Add it into the free page list
    setNextFreePageOffset(&buf, getNextFreePageOffset(&header));
    setNextFreePageOffset(&header, OFFSET(pagenum));

    // Apply changes to the header page.
    file_write_page(HEADER_PAGE_NUM, &header);
    file_write_page(pagenum, &buf);
}

/*
 *  Read an on-disk page into the in-memory page structure(dest)
 */
void file_read_page(pagenum_t pagenum, page_t* dest) {
    SEEK(OFFSET(pagenum));
    READ(*dest);
}

/*
 *  Write an in-memory page(src) to the on-disk page
 */
void file_write_page(pagenum_t pagenum, const page_t* src) {
    SEEK(OFFSET(pagenum));
    WRITE(*src);
}

/*
 *  Open existing data file using ‘pathname’ or create one if not existed.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int open_db (char *pathname) {
    if (pathname == NULL)
        return INVALID_FILENAME;

    if (fd != 0)
        return INVALID_FD; // FD is already open!

    // Make a file if it does not exist.
    // Do not use a symbolic link.
    // Flush the buffer whenever try to write.
    // Permission is set to 0644.
    fd = open(pathname, O_CREAT | O_RDWR | O_NOFOLLOW | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );

    // Add a handler which is to be executed before the process exits.
    atexit(when_exit);

    if (fd == -1) {
        perror("open_db error");
        return INVALID_FD;
    }

    // Read the header from the file
    if (READ(header) == 0) {
        // If the header page doesn't exist, create a new one.
        CLEAR(header);
        setNumOfPages(&header, 1); // the number of created pages
        WRITE(header);
    }

    return SUCCESS;
}

/*
 *  Flush and Sync the buffer and exit.
 */
void when_exit(void) {
    fsync(fd);
    if (fd != 0)
        close(fd);
}

// Getters and Setters

// for Header Page
// -> Getters
offset_t getFreePageOffset(const page_t *page) {
    return ((const header_page_t*)page)->free_page_offset;
}
offset_t getRootPageOffset(const page_t *page) {
    return ((const header_page_t*)page)->root_page_offset;
}
pagenum_t getNumOfPages(const page_t *page) {
    return ((const header_page_t*)page)->num_of_page;
}
// -> Setters
int setFreePageOffset(page_t *page, offset_t free_page_offset) {
    ((header_page_t*)page)->free_page_offset = free_page_offset;
    return SUCCESS;
}
int setRootPageOffset(page_t *page, offset_t root_page_offset) {
    ((header_page_t*)page)->root_page_offset = root_page_offset;
    return SUCCESS;
}
int setNumOfPages(page_t *page, pagenum_t num_of_page) {
    ((header_page_t*)page)->num_of_page = num_of_page;
    return SUCCESS;
}

// for Free Page
// -> Getters
offset_t getNextFreePageOffset(const page_t *page) {
    return ((const free_page_t*)page)->next_free_page_offset;
}
// -> Setters
int setNextFreePageOffset(page_t *page, offset_t next_free_page_offset) {
    ((free_page_t*)page)->next_free_page_offset = next_free_page_offset;
    return SUCCESS;
}

// for Node Page
// -> Getters
offset_t getParentOffset(const page_t *page) {
    return ((const node_page_t*)page)->offset;
}
int isLeaf(const page_t *page) {
    return ((const node_page_t*)page)->isLeaf;
}
int getNumOfKeys(const page_t *page) {
    return ((const node_page_t*)page)->num_keys;
}
int getKey(const page_t *page, uint32_t index) {
    if (!isLeaf(page)) {
        // Internal Page : 0 ≤ index < DEFAULT_INTERNAL_ORDER - 1
        if (index >= DEFAULT_INTERNAL_ORDER - 1 || index < 0) {
            return INVALID_INDEX;
        } else {
            return ((const node_page_t*)page)->pairs[index].key;
        }
    } else {
        // Leaf Page : 0 ≤ index < DEFAULT_LEAF_ORDER - 1
        if (index >= DEFAULT_LEAF_ORDER - 1 || index < 0)
            return INVALID_INDEX;
        else
            return ((const node_page_t*)page)->records[index].key;
    }
}
offset_t getOffset(const page_t *page, uint32_t index) {
    if (!isLeaf(page)) {
        // Internal Page : 0 ≤ index < DEFAULT_INTERNAL_ORDER
        if (index >= DEFAULT_INTERNAL_ORDER | index < 0)
            return INVALID_INDEX;
        else if (index == 0)
            return ((const node_page_t*)page)->one_more_page_offset;
        else
            return ((const node_page_t*)page)->pairs[index - 1].offset;
    } else {
        // Leaf Page : index = DEFAULT_LEAF_ORDER - 1
        if (index == DEFAULT_LEAF_ORDER)
            return ((const node_page_t*)page)->right_sibling_page_offset;
        else
            return INVALID_INDEX;
    }
}
int getValue(const page_t *page, uint32_t index, char *desc) {
    if (isLeaf(page)) {
        // Leaf Page : 0 ≤ index < DEFAULT_LEAF_ORDER - 1
        if (index >= DEFAULT_LEAF_ORDER || index < 0) {
            return INVALID_INDEX;
        }
        strncpy(desc, ((const node_page_t*)page)->records[index].value, 120);
        return SUCCESS;
    } else {
        return INVALID_PAGE;
    }
}
// -> Setters
int setParentOffset(page_t *page, offset_t offset) {
    ((node_page_t*)page)->offset = offset;
    return SUCCESS;
}
int setLeaf(page_t *page) {
    ((node_page_t*)page)->isLeaf = true;
    return SUCCESS;
}
int setInternal(page_t *page) {
    ((node_page_t*)page)->isLeaf = false;
    return SUCCESS;
}
int setNumOfKeys(page_t *page, uint32_t num_keys) {
    ((node_page_t*)page)->num_keys = num_keys;
    return SUCCESS;
}
int setKey(page_t *page, uint32_t index, uint64_t key) {
    if (!isLeaf(page)) {
        // Internal Page : 0 ≤ index < DEFAULT_INTERNAL_ORDER - 1
        if (index >= DEFAULT_INTERNAL_ORDER - 1)
            return INVALID_INDEX;

        ((node_page_t*)page)->pairs[index].key = key;
        return SUCCESS;
    } else {
        // Leaf Page : 0 ≤ index < DEFAULT_LEAF_ORDER - 1
        if (index >= DEFAULT_LEAF_ORDER)
            return INVALID_INDEX;

        ((node_page_t*)page)->records[index].key = key;
    }
}
int setOffset(page_t *page, uint32_t index, uint64_t offset) {
    if (!isLeaf(page)) {
        // Internal Page : 0 ≤ index < DEFAULT_INTERNAL_ORDER
        if (index >= DEFAULT_INTERNAL_ORDER | index < 0)
            return INVALID_INDEX;
        else if (index == 0)
            ((node_page_t*)page)->one_more_page_offset = offset;
        else
            ((node_page_t*)page)->pairs[index - 1].offset = offset;
        return SUCCESS;
    } else {
        // Leaf Page : index == DEFAULT_LEAF_ORDER - 1
        if (index != DEFAULT_LEAF_ORDER - 1) {
            return INVALID_INDEX;
        } else {
            ((node_page_t*)page)->right_sibling_page_offset = offset;
            return SUCCESS;
        }
    }
}
int setValue(page_t *page, uint32_t index, const char *src) {
    if (isLeaf(page)) {
        // Leaf Page : 0 ≤ index < DEFAULT_LEAF_ORDER - 1
        if (index >= DEFAULT_LEAF_ORDER | index < 0) {
            return INVALID_INDEX;
        }

        strncpy(((node_page_t*)page)->records[index].value, src, 120);
        return SUCCESS;
    } else {
        return INVALID_PAGE;
    }
}

offset_t find_leaf( offset_t root, uint64_t key ) {
    offset_t c = root;
    if (c == HEADER_PAGE_OFFSET) {
        return c;
    }

    page_t buf;

    while (true) {
        file_read_page(PGNUM(c), &buf);
        // If the page is leaf,
        if (isLeaf(&buf))
            break; // break the loop.

        // If the page is internal,
        /* If the given key is smaller than the least key in the page */
        if (key < getKey(&buf, 0)) {
            // Set the search offset to be the one-more-page-leaf-offset.
            c = getOffset(&buf, 0);
        } else {
            int index = binaryRangeSearch(&buf, key);
            if (index == INVALID_KEY)
                return INVALID_KEY;
            // Right side of the index
            c = getOffset(&buf, index + 1);
        }
    }
    return c;
}

record_t * find_record( offset_t root, uint64_t key ) {
    offset_t c = find_leaf( root, key );

    if (c == HEADER_PAGE_OFFSET) {
        return NULL;
    }

    page_t leaf_page;
    file_read_page(PGNUM(c), &leaf_page);

    int index = binarySearch(&leaf_page, key);

    if (index == INVALID_KEY)
        return NULL;

    record_t* ret = malloc( sizeof(record_t) );
    if (!ret)
        return NULL;

    ret->key = getKey(&leaf_page, index);
    getValue(&leaf_page, index, ret->value);
    return ret;
}

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

    /* Case: the tree already exists.
     * (Rest of function body.)
     */

    offset_t leaf = find_leaf(root, record->key);

    file_read_page(PGNUM(leaf), &buf);

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

/* Finds the appropriate place to
 * split a node that is too big into two.
 */
int cut( int length ) {
    if (length % 2 == 0)
        return length / 2;
    else
        return length / 2 + 1;
}

int binarySearch(const page_t *page, uint64_t key) {
    int l = 0, r = getNumOfKeys(page) - 1, m, middle_key;
    while (l <= r) {
        m = (l + r) / 2;
        middle_key = getKey(page, m);
        if (middle_key < key)
            l = m + 1;
        else if (middle_key > key)
            r = m - 1;
        else
            return m;
    }
    return INVALID_KEY; /* cannot find the given key in the page */
}

int binaryRangeSearch(const page_t *page, uint64_t key) {
    /* Modified Version of Binary Search */
    int l = 0, r = getNumOfKeys(page), m, middle_key;
    while (l < r) {
        m = (l + r) / 2;

        if ( l == m )
            return m;

        middle_key = getKey(page, m);
        if (middle_key <= key)
            l = m;
        else
            r = m;
    }
    return INVALID_KEY; /* No Key is provided. */
}

void enqueue( offset_t new_node_offset, int depth ) {
    node *c, *new_node = malloc(sizeof(node));

    if (!new_node)
        return;

    new_node->offset = new_node_offset;
    new_node->depth = depth;
    new_node->next = NULL;

    if (queue == NULL) {
        queue = new_node;
    } else {
        c = queue;
        while (c->next != NULL) {
            c = c->next;
        }
        c->next = new_node;
    }
}

offset_t dequeue( int *depth ) {
    if (!queue)
        return HEADER_PAGE_OFFSET;

    offset_t ret;
    node * n;

    n = queue;
    queue = queue->next;
    ret = n->offset;
    if (depth)
        *depth = n->depth;
    n->next = NULL;

    free(n);
    return ret;
}

void print_leaves( offset_t root ) {
    int i, num_keys;
    offset_t c = root;
    page_t buf;
    if (root == HEADER_PAGE_OFFSET) {
        printf("Empty tree.\n");
        return;
    }
    file_read_page(PGNUM(c), &buf);
    while (!isLeaf(&buf))
        c = getOffset(&buf, 0);
    while (true) {
        for (i = 0, num_keys = getNumOfKeys(&buf); i < num_keys; i++) {
            printf("%d ", getKey(&buf, i));
        }
        if (getOffset(&buf, DEFAULT_LEAF_ORDER - 1) != HEADER_PAGE_OFFSET) {
            printf(" | ");
            c = getOffset(&buf, DEFAULT_LEAF_ORDER - 1);
        }
        else
            break;
    }
    printf("\n");
}

void print_tree ( offset_t root ) {
    int i = 0, search_level = 0, node_level;
    int num_keys;

    page_t buf;
    offset_t c;

    if (root == HEADER_PAGE_OFFSET) {
        printf("Empty tree.\n");
        return;
    }

    queue = NULL;
    enqueue(root, search_level);
    while ( queue != NULL ) {
        c = dequeue(&node_level);
        file_read_page(PGNUM(c), &buf);

        if (search_level < node_level) {
            puts("");
            search_level = node_level;
        }

        num_keys = getNumOfKeys(&buf);
        for (i = 0; i < num_keys; i++) {
            printf("%d ", getKey(&buf, i));
        }
        if (!isLeaf(&buf))
            for (i = 0; i <= num_keys; i++)
                enqueue(getOffset(&buf, i), node_level + 1);
        printf("| ");
    }
    printf("\n");
}

void usage(void) {
    printf( "Enter any of the following commands after the prompt > :\n"
            "\ti <k>  -- Insert <k> (an integer) as both key and value).\n"
            "\tf <k>  -- Find the value under key <k>.\n"
            "\tp <k> -- Print the path from the root to key k and its associated value.\n"
            "\tr <k1> <k2> -- Print the keys and values found in the range [<k1>, <k2>]\n"
            "\td <k>  -- Delete key <k> and its associated value.\n"
            "\tx -- Destroy the whole tree.  Start again with an empty tree of the same order.\n"
            "\tt -- Print the B+ tree.\n"
            "\tl -- Print the keys of the leaves (bottom row of the tree).\n"
            "\tv -- Toggle output of pointer addresses (\"verbose\") in tree and leaves.\n"
            "\tq -- Quit. (Or use Ctl-D.)\n"
            "\t? -- Print this help message.\n");
}

// GLOBALS

/*
 * The queue is used to print the tree in level order,
 * starting from the root printing each entire rank on a separate line,
 * finishing with the leaves.
 */
node* queue;

/*
 * File Descriptor for R/W
 */
int fd;

/*
 * Cached Header Page
 */
page_t header;