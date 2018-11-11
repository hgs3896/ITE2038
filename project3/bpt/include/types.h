#ifndef __TYPES_H__
#define __TYPES_H__

#include "constants.h"

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
    int depth;
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
        key_pair pairs[DEFAULT_INTERNAL_ORDER-1];  // for internal page
        // Data Records
        record_t records[DEFAULT_LEAF_ORDER-1]; // for leaf page
    };
};

// Universal Page Layout
union page_t
{
    header_page_t header_page;
    free_page_t free_page;
    node_page_t node_page;
};

typedef struct buffer_frame_t{
    /* • Physical frame: containing up to date contents of target page. */
    page_t frame;
    /* • Table id: the unique id of table (per file) */
    int table_id;
    /* • Page number: the target page number within a file. */
    pagenum_t pgnum;
    /* • Is dirty: whether this buffer block is dirty or not. */
    bool dirty;
    /* • Is pinned: whether this buffer is accessed right now. */
    int pin_cnt;
    /* • LRU list next (prev) : buffer blocks are managed by LRU list. */
    struct buffer_frame_t next, prev;
    /* • Other information can be added with your own buffer manager design. */
    // Empty
};

#endif