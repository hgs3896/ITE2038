#ifndef __BPT_H__
#define __BPT_H__

#define Version "1.14"

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

// Default order is 4.
#define DEFAULT_ORDER 4

// Page size
#define PAGESIZE 4096

// Default size of free pages
#define DEFAULT_SIZE_OF_FREE_PAGES 10

// Minimum order is necessarily 3.  We set the maximum
// order arbitrarily.  You may change the maximum order.
#define MIN_ORDER 3
#define MAX_ORDER 20

// Constants for printing part or all of the GPL license.
#define LICENSE_FILE "LICENSE.txt"
#define LICENSE_WARRANTEE 0
#define LICENSE_WARRANTEE_START 592
#define LICENSE_WARRANTEE_END 624
#define LICENSE_CONDITIONS 1
#define LICENSE_CONDITIONS_START 70
#define LICENSE_CONDITIONS_END 625

// MACROs
#define READ(buf) (read(fd, &buf, PAGESIZE))
#define WRITE(buf) (write(fd, &buf, PAGESIZE))
#define CLEAR(buf) (memset(&buf, 0, PAGESIZE))
#define SEEK(offset) (lseek(fd, offset, SEEK_SET) >= 0)
#define OFFSET(page_num) (pagenum * PAGESIZE)

// TYPES.

/* Type representing the record
 * to which a given key refers.
 * In a real B+ tree system, the
 * record would hold data (in a database)
 * or a file (in an operating system)
 * or some other information.
 * Users can rewrite this part of the code
 * to change the type and content
 * of the value field.
 */
typedef struct record {
    int key, value;
} record;

/* Type representing a node in the B+ tree.
 * This type is general enough to serve for both
 * the leaf and the internal node.
 * The heart of the node is the array
 * of keys and the array of corresponding
 * pointers.  The relation between keys
 * and pointers differs between leaves and
 * internal nodes.  In a leaf, the index
 * of each key equals the index of its corresponding
 * pointer, with a maximum of order - 1 key-pointer
 * pairs.  The last pointer points to the
 * leaf to the right (or NULL in the case
 * of the rightmost leaf).
 * In an internal node, the first pointer
 * refers to lower nodes with keys less than
 * the smallest key in the keys array.  Then,
 * with indices i starting at 0, the pointer
 * at i + 1 points to the subtree with keys
 * greater than or equal to the key in this
 * node at index i.
 * The num_keys field is used to keep
 * track of the number of valid keys.
 * In an internal node, the number of valid
 * pointers is always num_keys + 1.
 * In a leaf, the number of valid pointers
 * to data is always num_keys.  The
 * last leaf pointer points to the next leaf.
 */
typedef struct node {
    void ** pointers;
    int * keys;
    struct node * parent;
    bool is_leaf;
    int num_keys;
    struct node * next; // Used for queue.
} node;

// Page number
typedef uint64_t pagenum_t;
typedef uint64_t pageoffset_t;
typedef int8_t byte;

// Record format
typedef struct record_t
{
    uint64_t key;
    int8_t value[120];
} record_t;

// Key Pair
typedef struct key_pair
{
    uint64_t key;
    pageoffset_t offset;
} key_pair;

// Page Layout
typedef struct page_t
{
    // in-memory page structure
    union
    {
        struct header_page
        {
            pageoffset_t free_page_offset; // 8
            pageoffset_t root_page_offset; // 8
            pagenum_t num_of_page; // 8
            byte reserved[4072];
        } header_page; // 4096
        struct free_page
        {
            pageoffset_t next_free_page_offset; // 8
            byte not_used[4088];
        } free_page; // 4096
        struct
        {
            struct
            {
                pageoffset_t offset; // 8
                uint32_t isLeaf; // 4
                uint32_t num_of_keys; // 4
                byte reserved[104]; // 104
                // Right Sibling Page Offset
                pageoffset_t right_sibling_page_offset; // 8
            } page_header; // 128
            record_t data[31]; // 31 * 128 = 3968
        } leaf_page; // 4096
        struct internal_page
        {
            struct page_header
            {
                pageoffset_t offset; // 8
                uint32_t isLeaf; // 4
                uint32_t num_of_keys; // 4
                byte reserved[104]; // 104
                // One More Page Offset
                pageoffset_t one_more_page_offset; // 8
            } page_header;
            key_pair pairs[248];
        } internal_page;
    };
} page_t;

// GLOBALS.

/* The order determines the maximum and minimum
 * number of entries (keys and pointers) in any
 * node.  Every node has at most order - 1 keys and
 * at least (roughly speaking) half that number.
 * Every leaf has as many pointers to data as keys,
 * and every internal node has one more pointer
 * to a subtree than the number of keys.
 * This global variable is initialized to the
 * default value.
 */
extern int order;

/* The queue is used to print the tree in
 * level order, starting from the root
 * printing each entire rank on a separate
 * line, finishing with the leaves.
 */
extern node * queue;

/* The user can toggle on and off the "verbose"
 * property, which causes the pointer addresses
 * to be printed out in hexadecimal notation
 * next to their corresponding keys.
 */
extern bool verbose_output;

/* File descriptor for reading and writing database file. */
extern int fd;

/* Header Page */
extern page_t header;


extern const pagenum_t PAGE_NOT_FOUND;

// FUNCTION PROTOTYPES.

// Output and utility.

void license_notice( void );
void print_license( int licence_part );
void usage_1( void );
void usage_2( void );
void usage_3( void );
void enqueue( node * new_node );
node * dequeue( void );
void print_leaves( node * root );
void print_tree( node * root );


int height( node * root );
int path_to_root( node * root, node * child );

void find_and_print(node * root, int key, bool verbose); 
void find_and_print_range(node * root, int range1, int range2, bool verbose); 
int find_range( node * root, int key_start, int key_end, bool verbose,
        int returned_keys[], void * returned_pointers[]); 
node * find_leaf( node * root, int key, bool verbose );
record * find( node * root, int key, bool verbose );
int cut( int length );

// Insertion.

record * make_record(int value);
node * make_node( void );
node * make_leaf( void );
int get_left_index(node * parent, node * left);
node * insert_into_leaf( node * leaf, int key, record * pointer );
node * insert_into_leaf_after_splitting(node * root, node * leaf, int key,
                                        record * pointer);
node * insert_into_node(node * root, node * parent, 
        int left_index, int key, node * right);
node * insert_into_node_after_splitting(node * root, node * parent,
                                        int left_index,
        int key, node * right);
node * insert_into_parent(node * root, node * left, int key, node * right);
node * insert_into_new_root(node * left, int key, node * right);
node * start_new_tree(int key, record * pointer);
node * insert( node * root, int key, int value );

// Deletion.

int get_neighbor_index( node * n );
node * adjust_root(node * root);
node * coalesce_nodes(node * root, node * n, node * neighbor,
                      int neighbor_index, int k_prime);
node * redistribute_nodes(node * root, node * n, node * neighbor,
                          int neighbor_index,
        int k_prime_index, int k_prime);
node * delete_entry( node * root, node * n, int key, void * pointer );
node * delete( node * root, int key );

void destroy_tree_nodes(node * root);
node * destroy_tree(node * root);

// File Management

void when_exit(void);

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page();
// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum);
// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest);
// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src);

/*
    Open existing data file using ‘pathname’ or create one if not existed.
    If success, return 0. Otherwise, return non-zero value.
*/
int open_db (char *pathname);
/*
    Insert input ‘key/value’ (record) to data file at the right place.
    If success, return 0. Otherwise, return non-zero value.
*/
// int insert (int64_t key, char * value);
/*
    Find the record containing input ‘key’.
    If found matching ‘key’, return matched ‘value’ string. Otherwise, return NULL.
*/
// char * find (int64_t key);
/*
    Find the matching record and delete it if found.
    If success, return 0. Otherwise, return non-zero value.
*/
// int delete (int64_t key);

#endif /* __BPT_H__*/
