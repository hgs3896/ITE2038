#include "bpt.h"

/*
    Find the record containing input ‘key’.
    If found matching ‘key’, return matched ‘value’ string. Otherwise, return NULL.
*/
char * find (int64_t key) {
  record_t* record = find_record(getRootPageOffset(&header), key);
  if (record == NULL) return NULL;
  char * ret = malloc(sizeof(record_t) - sizeof(keynum_t));
  strncpy(ret, record->value, sizeof(record_t) - sizeof(keynum_t));
  free(record);
  return ret;
}

/*
 *  Insert input ‘key/value’ (record) to data file at the right place.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int insert (int64_t key, char * value) {
  record_t record;
  record.key = key;
  strncpy(record.value, value, 120);

  file_read_page(HEADER_PAGE_NUM, &header);
  offset_t insert_offset = insert_record(getRootPageOffset(&header), &record);
  return insert_offset == KEY_EXIST;
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
uint32_t isLeaf(const page_t *page) {
  return ((const node_page_t*)page)->isLeaf;
}
uint32_t getNumOfKeys(const page_t *page) {
  return ((const node_page_t*)page)->num_keys;
}
uint64_t getKey(const page_t *page, uint32_t index) {
  if (!isLeaf(page)) {
    // Internal Page : 0<=index<DEFAULT_INTERNAL_ORDER
    if (index >= DEFAULT_INTERNAL_ORDER - 1) {
      return INVALID_INDEX;
    } else {
      return ((const node_page_t*)page)->pairs[index].key;
    }
  } else {
    // Leaf Page : 0<=index<DEFAULT_LEAF_ORDER - 1
    if (index >= DEFAULT_LEAF_ORDER)
      return INVALID_INDEX;
    else
      return ((const node_page_t*)page)->records[index].key;
  }
}
uint64_t getOffset(const page_t *page, uint32_t index) {
  if (!isLeaf(page)) {
    // Internal Page : 0<=index<DEFAULT_INTERNAL_ORDER
    if (index >= DEFAULT_INTERNAL_ORDER)
      return INVALID_INDEX;
    else if (index == 0)
      return ((const node_page_t*)page)->one_more_page_offset;
    else
      return ((const node_page_t*)page)->pairs[index - 1].offset;
  } else {
    // Leaf Page : DEFAULT_LEAF_ORDER - 1
    if (index >= DEFAULT_LEAF_ORDER)
      return INVALID_INDEX;
    else if (index == DEFAULT_LEAF_ORDER - 1)
      return ((const node_page_t*)page)->right_sibling_page_offset;
    else
      return INVALID_PAGE;
  }
}
int getValue(const page_t *page, uint32_t index, char *desc) {
  if (isLeaf(page)) {
    // Leaf Page : 0<=index<DEFAULT_LEAF_ORDER - 1
    if (index >= DEFAULT_LEAF_ORDER) {
      return INVALID_INDEX;
    }

    strncpy(desc, ((const node_page_t*)page)->records[index].value, sizeof(record_t) - sizeof(keynum_t));
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
    // Internal Page : 0<=index<DEFAULT_INTERNAL_ORDER
    if (index >= DEFAULT_INTERNAL_ORDER - 1)
      return INVALID_INDEX;

    ((node_page_t*)page)->pairs[index].key = key;
    return SUCCESS;
  } else {
    // Leaf Page : 0<=index<DEFAULT_LEAF_ORDER - 1
    if (index >= DEFAULT_LEAF_ORDER)
      return INVALID_INDEX;

    ((node_page_t*)page)->records[index].key = key;;
  }
}
int setOffset(page_t *page, uint32_t index, uint64_t offset) {
  if (!isLeaf(page)) {
    // Internal Page : 0<=index<=DEFAULT_INTERNAL_ORDER
    if (index >= DEFAULT_INTERNAL_ORDER)
      return INVALID_INDEX;
    else if (index == 0)
      ((node_page_t*)page)->one_more_page_offset = offset;
    else
      ((node_page_t*)page)->pairs[index - 1].offset = offset;
    return SUCCESS;
  } else {
    // Leaf Page : DEFAULT_LEAF_ORDER - 1
    if (index >= DEFAULT_LEAF_ORDER)
      return INVALID_INDEX;
    else if (index == DEFAULT_LEAF_ORDER - 1)
      return ((node_page_t*)page)->right_sibling_page_offset = offset;
    else
      return INVALID_PAGE;
  }
}
int setValue(page_t *page, uint32_t index, const char *src) {
  if (isLeaf(page)) {
    // Leaf Page : 0<=index<DEFAULT_LEAF_ORDER - 1
    if (index >= DEFAULT_LEAF_ORDER) {
      return INVALID_INDEX;
    }

    strncpy(((node_page_t*)page)->records[index].value, src, sizeof(record_t) - sizeof(keynum_t));
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
      uint64_t index = modified_binary_search_in_page(&buf, key);
      if (index == INVALID_KEY)
        return INVALID_KEY;
      c = getOffset(&buf, index);
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
  uint64_t index = binary_search_in_page(&leaf_page, key);

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
  offset_t new_page_offset = file_alloc_page();
  page_t buf;
  file_read_page(PGNUM(new_page_offset), &buf);
  CLEAR(buf);
  // setInternal(&buf);
  file_write_page(PGNUM(new_page_offset), &buf);
  return new_page_offset;
}

/*
 * Make a leaf page and return its offset.
 */
offset_t make_leaf( void ) {
  offset_t new_page_offset = file_alloc_page();
  page_t buf;
  file_read_page(PGNUM(new_page_offset), &buf);
  CLEAR(buf);
  setLeaf(&buf);
  file_write_page(PGNUM(new_page_offset), &buf);
  return new_page_offset;
}

/*
 * Get the index of a left page in terms of a parent page.
 */
// int get_left_index( offset_t parent, offset_t left);

/*
 * Insert a record into a leaf page which has a room to save a record.
 */
offset_t insert_into_leaf( offset_t leaf, const record_t * pointer ) {
  char buf_value[120];
  int i, insertion_point, num_keys;
  page_t buf;

  file_read_page(PGNUM(leaf), &buf);
  num_keys = getNumOfKeys(&buf);
  uint64_t index = modified_binary_search_in_page(&buf, pointer->key);
  if (index == INVALID_KEY)
    return INVALID_KEY;

  insertion_point = index + 1;

  for (i = num_keys; i > insertion_point; i--) {
    setKey(&buf, i, getKey(&buf, i - 1));
    getValue(&buf, i - 1, buf_value);
    setValue(&buf, i, buf_value);
  }
  setKey(&buf, insertion_point, pointer->key); // leaf->keys[insertion_point] = key;
  setValue(&buf, insertion_point, pointer->value); // leaf->pointers[insertion_point] = pointer;

  setNumOfKeys(&buf, getNumOfKeys(&buf) + 1);

  file_write_page(PGNUM(leaf), &buf);
  return leaf;
}

/*
 * Insert a record into a leaf page which has no room to save more records.
 * Therefore, first split the leaf page and call insert_into_parent() for pushing up the key.
 */
// offset_t insert_into_leaf_after_splitting( offset_t root, offset_t leaf, record_t * record);

/*
 * Insert a key into an internal page to save a record.
 * To save the key, it has to determine whether to split and whether to push a key.
 */
// offset_t insert_into_parent( offset_t root, offset_t left, uint64_t key, offset_t right );

/*
 * Insert a key into a internal page which has a room to save a key.
 */
// offset_t insert_into_node( offset_t root, offset_t parent, int left_index, uint64_t key, offset_t right);

/*
 * Insert a key into a internal page which has no room to save more keys.
 * Therefore, first split the internal page and call insert_into_parent() for pushing up the key recursively.
 */
// offset_t insert_into_node_after_splitting( offset_t root, offset_t parent, int left_index, uint64_t key, offset_t right);

/*
 * Make a new root page for insertion of the pushup key and insert it into the root page.
 */
// offset_t insert_into_new_root( offset_t left, uint64_t key, offset_t right );

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

  file_read_page(HEADER_PAGE_NUM, &header);
  setRootPageOffset(&header, root);
  file_write_page(HEADER_PAGE_NUM, &header);
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

  /* The current implementation ignores duplicates.
   */

  if ((pointer = find_record(root, record->key)) != NULL) {
    free(pointer);
    return KEY_EXIST; // Duplicated
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

  page_t buf;
  file_read_page(PGNUM(leaf), &buf);

  /* Case: leaf has room for key and pointer.
   */

  if (getNumOfKeys(&buf) < DEFAULT_LEAF_ORDER - 1) {
    leaf = insert_into_leaf(leaf, record);
    return root;
  }
#if 0
  /* Case:  leaf must be split.
   */

  return insert_into_leaf_after_splitting(root, leaf, record);
#endif
  return 0;
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

uint64_t binary_search_in_page(const page_t *page, uint64_t key) {
  uint64_t l = 0, r = getNumOfKeys(page) - 1, m, middle_key;
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

uint64_t modified_binary_search_in_page(const page_t *page, uint64_t key) {
  /* Modified Version of Binary Search */
  uint64_t l = 0, r = getNumOfKeys(page), m, middle_key;
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

void enqueue( offset_t new_node ) {
  node* c;
  if (queue == NULL) {
    queue = malloc(sizeof(node));
    queue->offset = new_node;
    queue->next = NULL;
  }
  else {
    c = queue;
    while (c->next != NULL) {
      c = c->next;
    }
    c->next = malloc(sizeof(node));
    c->next->offset = new_node;
    c->next->next = NULL;
  }
}

offset_t dequeue( void ) {
  if (!queue) return HEADER_PAGE_OFFSET;
  node * n = queue;
  queue = queue->next;
  n->next = NULL;
  offset_t ret = n->offset;
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
      printf("%lu ", getKey(&buf, i));
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
  int i = 0;
  int num_keys;

  page_t buf;
  offset_t c;

  if (root == HEADER_PAGE_OFFSET) {
    printf("Empty tree.\n");
    return;
  }
  queue = NULL;
  enqueue(root);
  while ( queue != NULL ) {
    c = dequeue();
    file_read_page(PGNUM(c), &buf);
    num_keys = getNumOfKeys(&buf);
    for (i = 0; i < num_keys; i++) {
      printf("%lu ", getKey(&buf, i));
    }
    if (!isLeaf(&buf))
      for (i = 0; i <= num_keys; i++)
        enqueue(getOffset(&buf, i));
    printf("| ");
  }
  printf("\n");
}

void usage(void) {
  printf( "Enter any of the following commands after the prompt > :\n"
          "\ti <k>  -- Insert <k> (an integer) as both key and value).\n"
          "\tf <k>  -- Find the value under key <k>.\n"
          "\tp <k> -- Print the path from the root to key k and its associated value.\n"
          "\tr <k1> <k2> -- Print the keys and values found in the range [<k1>, <k2>\n"
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