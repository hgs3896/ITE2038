#ifndef __BPT_H__
#define __BPT_H__

#define Version "1.00"

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

// CONSTANTS

#define DEFAULT_INTERNAL_ORDER 249
#define DEFAULT_LEAF_ORDER 32

// Page size
#define PAGESIZE 4096

// Default size of free pages
#define DEFAULT_SIZE_OF_FREE_PAGES 10

/* Error Code */
#define SUCCESS            0
#define INVALID_OFFSET    -1
#define INVALID_INDEX     -2
#define INVALID_KEY       -3
#define INVALID_PAGE      -4
#define INVALID_FD        -5
#define INVALID_FILENAME  -6
#define READ_ERROR        -7
#define WRITE_ERROR       -8
#define SEEK_ERROR        -9

/* Exception Code */
#define KEY_EXIST         -100
#define KEY_NOT_FOUND     -101


/* Offset */
#define HEADER_PAGE_NUM 0
#define HEADER_PAGE_OFFSET 0

// TYPES.

// Type Redefinition
typedef int8_t byte;
typedef uint64_t keynum_t;
typedef uint64_t pagenum_t;
typedef uint64_t offset_t;
typedef struct node node;
typedef struct record_t record_t;
typedef struct key_pair key_pair;
typedef struct header_page_t header_page_t;
typedef struct free_page_t free_page_t;
typedef struct node_page_t node_page_t;
typedef  union page_t page_t;

// Type Definition

/* Node format for using queue */
struct node
{
    offset_t offset;
    struct node * next; // Used for queue.
};

/* Record format for storing the records */
struct record_t
{
    keynum_t key;
    byte value[120];
};

/* Key Pair */
struct key_pair
{
    keynum_t key;
    offset_t offset;
};

/* Header Page Layout */
struct header_page_t
{
    offset_t free_page_offset;
    offset_t root_page_offset;
    pagenum_t num_of_page;
    byte reserved[4072];
};

/* Free Page Layout */
struct free_page_t
{
    offset_t next_free_page_offset;
    byte not_used[4088];
};

/* Internal/Leaf Node Page Layout */
struct node_page_t
{
    offset_t offset;    // Parent page offset
    uint32_t isLeaf;    // Leaf(1) / Internal(0)
    uint32_t num_keys;  // The number of keys : Maximum Internal Page(249), Leaf Page(31)
    byte reserved[104]; // Reserved Area
    union
    {
        // One More Page Offset
        offset_t one_more_page_offset; // (leftmost child) for internal page
        // Right Sibling Page Offset
        offset_t right_sibling_page_offset; // (next page offset) for leaf page
    };
    union
    {
        // Key Pairs
        key_pair pairs[248];  // for internal page
        // Data Records
        record_t records[31]; // for leaf page
    };
};

// Universal Page Layout
union page_t
{
    header_page_t header_page;
    free_page_t free_page;
    node_page_t node_page;
};

// MACROs for convinience
#define READ(buf) (read(fd, &(buf), PAGESIZE))
#define WRITE(buf) (write(fd, &(buf), PAGESIZE))
#define PAGE_ALLOC() (malloc(PAGE_SIZE))
#define PAGE_FREE(buf_addr) (free(buf_addr))
#define CLEAR(buf) (memset(&(buf), 0, PAGESIZE))
#define SEEK(offset) (lseek(fd, (offset), SEEK_SET) >= 0)
#define OFFSET(pagenum) ((pagenum) * PAGESIZE)
#define PGNUM(pageoffset) ((pageoffset) / PAGESIZE)

// File Manager APIs

/*
    Find the record containing input ‘key’.
    If found matching ‘key’, return matched ‘value’ string. Otherwise, return NULL.
*/
char * find (int64_t key);

/*
 *  Insert input ‘key/value’ (record) to data file at the right place.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int insert (int64_t key, char * value);

/*
 *  Allocate an on-disk page from the free page list
 */
pagenum_t file_alloc_page();

/*
 *  Free an on-disk page to the free page list
 */
void file_free_page(pagenum_t pagenum);

/*
 *  Read an on-disk page into the in-memory page structure(dest)
 */
void file_read_page(pagenum_t pagenum, page_t* dest);

/*
 *  Write an in-memory page(src) to the on-disk page
 */
void file_write_page(pagenum_t pagenum, const page_t* src);

/*
 *  Open existing data file using ‘pathname’ or create one if not existed.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int open_db (char *pathname);

/*
 *  Find the matching record and delete it if found.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int delete (int64_t key);

/*
 *  Flush and Sync the buffer and exit.
 */
void when_exit(void);

// Getters and Setters

// for Header Page
// -> Getters
offset_t getFreePageOffset(const page_t *page);
offset_t getRootPageOffset(const page_t *page);
pagenum_t getNumOfPages(const page_t *page);
// -> Setters
int setFreePageOffset(page_t *page, offset_t free_page_offset);
int setRootPageOffset(page_t *page, offset_t root_page_offset);
int setNumOfPages(page_t *page, pagenum_t num_of_page);

// for Free Page
// -> Getters
offset_t getNextFreePageOffset(const page_t *page);
// -> Setters
int setNextFreePageOffset(page_t *page, offset_t next_free_page_offset);

// for Node Page
// -> Getters
offset_t getParentOffset(const page_t *page);
uint32_t isLeaf(const page_t *page);
uint32_t getNumOfKeys(const page_t *page);
uint64_t getKey(const page_t *page, uint32_t index);
uint64_t getOffset(const page_t *page, uint32_t index);
int getValue(const page_t *page, uint32_t index, char *desc);
// -> Setters
int setParentOffset(page_t *page, offset_t offset);
int setLeaf(page_t *page);
int setInternal(page_t *page);
int setNumOfKeys(page_t *page, uint32_t num_keys);
int setKey(page_t *page, uint32_t index, uint64_t key);
int setOffset(page_t *page, uint32_t index, uint64_t offset);
int setValue(page_t *page, uint32_t index, const char *src);

// Utility

int cut( int length );
uint64_t binary_search_in_page(const page_t *page, uint64_t key);
uint64_t modified_binary_search_in_page(const page_t *page, uint64_t key);
void enqueue( offset_t new_node );
offset_t dequeue( void );
void print_leaves( offset_t root );
void print_tree( offset_t root );

// Search

/* Find where the given key is located and return the leaf page offset which it has the key.*/
offset_t find_leaf( offset_t root, uint64_t key );
record_t * find_record( offset_t root, uint64_t key ); // needs to be free after the call

// Insertion

/*
 * Make an intenal page and return its offset.
 */
offset_t make_internal( void );

/*
 * Make a leaf page and return its offset.
 */
offset_t make_leaf( void );

/*
 * Get the index of a left page in terms of a parent page.
 */
int get_left_index( offset_t parent, offset_t left);

/*
 * Insert a record into a leaf page which has a room to save a record.
 */
offset_t insert_into_leaf( offset_t leaf, const record_t * pointer );

/*
 * Insert a record into a leaf page which has no room to save more records.
 * Therefore, first split the leaf page and call insert_into_parent() for pushing up the key.
 */
offset_t insert_into_leaf_after_splitting( offset_t root, offset_t leaf, const record_t * record);

/*
 * Insert a key into an internal page to save a record.
 * To save the key, it has to determine whether to split and whether to push a key.
 */
offset_t insert_into_parent( offset_t root, offset_t left, uint64_t key, offset_t right );

/*
 * Insert a key into a internal page which has a room to save a key.
 */
offset_t insert_into_node( offset_t root, offset_t parent, int left_index, uint64_t key, offset_t right);

/*
 * Insert a key into a internal page which has no room to save more keys.
 * Therefore, first split the internal page and call insert_into_parent() for pushing up the key recursively.
 */
offset_t insert_into_node_after_splitting( offset_t root, offset_t parent, int left_index, uint64_t key, offset_t right);

/*
 * Make a new root page for insertion of the pushup key and insert it into the root page.
 */
offset_t insert_into_new_root( offset_t left, uint64_t key, offset_t right );

/*
 * If there is no tree, make a new tree.
 */
offset_t start_new_tree( const record_t* record );

/*
 * Insert a record into the disk-based B+tree.
 */
offset_t insert_record( offset_t root, const record_t * record );

/*
 * Print usage about the commands used to control the disk-based B+tree.
 */
void usage(void);

// GLOBALS

/*
 * The queue is used to print the tree in level order,
 * starting from the root printing each entire rank on a separate line,
 * finishing with the leaves.
 */
extern node* queue;

/*
 * File Descriptor for R/W
 */
extern int fd;

/*
 * Cached Header Page
 */
extern page_t header;

#endif /* __BPT_H__ */