#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

// TYPES.

// Page number
typedef int8_t byte;
typedef uint64_t pagenum_t;
typedef uint64_t offset_t;

typedef struct node
{
    offset_t offset;
    struct node * next; // Used for queue.
} node;

typedef struct record_t record_t;
typedef struct key_pair key_pair;
typedef struct header_page_t header_page_t;
typedef struct free_page_t free_page_t;
typedef struct node_page_t node_page_t;
typedef union page_t page_t;

// Record format
struct record_t
{
    uint64_t key;
    int8_t value[120];
};

// Key Pair
struct key_pair
{
    uint64_t key;
    offset_t offset;
};

struct header_page_t
{
    offset_t free_page_offset; // 8
    offset_t root_page_offset; // 8
    pagenum_t num_of_page; // 8
    byte reserved[4072];
};

struct free_page_t
{
    offset_t next_free_page_offset; // 8
    byte not_used[4088];
};

struct node_page_t
{
    offset_t offset; // Parent page offset
    uint32_t isLeaf; // Leaf(1) / Internal(0)
    uint32_t num_keys; // The number of keys, Internal Page(249), Leaf Page(31)
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

// Page Layout
union page_t
{
    header_page_t header_page;
    free_page_t free_page;
    node_page_t node_page;
};

#endif /* __TYPES_H__*/