#ifndef __BPT_H__
#define __BPT_H__

#define Version "1.14"

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include "types.h"
#include "globals.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

// MACROs
#define READ(buf) (read(fd, &(buf), PAGESIZE))
#define WRITE(buf) (write(fd, &(buf), PAGESIZE))
#define PAGE_ALLOC() (malloc(PAGE_SIZE))
#define PAGE_FREE(buf_addr) (free(buf_addr))
#define CLEAR(buf) (memset(&(buf), 0, PAGESIZE))
#define SEEK(offset) (lseek(fd, offset, SEEK_SET) >= 0)
#define OFFSET(page_num) (pagenum * PAGESIZE)

// FUNCTION PROTOTYPES.

// Output and utility.

void license_notice( void );
void print_license( int licence_part );
void usage_1( void );
void usage_2( void );
void usage_3( void );


void enqueue( offset_t new_node );
offset_t dequeue( void );
void print_leaves( offset_t * root );
void print_tree( pagenum_t pagenum );

void find_and_print(offset_t root, uint64_t key); 
void find_and_print_range(offset_t root, int range1, int range2); 
int find_range( offset_t root, uint64_t key_start, uint64_t key_end, int returned_keys[], void * returned_pointers[]); 
offset_t find_leaf( offset_t root, uint64_t key );
record_t * find( offset_t root, uint64_t key, );
int cut( int length );

// Insertion.

offset_t make_internal( void );
offset_t make_leaf( void );
int get_left_index(offset_t parent, offset_t left);
offset_t insert_into_leaf( offset_t leaf, uint64_t key, record_t * pointer );
offset_t insert_into_leaf_after_splitting( offset_t root, offset_t leaf, uint64_t key, record_t * pointer);
offset_t insert_into_node( offset_t root, offset_t parent, int left_index, uint64_t key, offset_t right);
offset_t insert_into_node_after_splitting( offset_t root, offset_t parent, int left_index, uint64_t key, offset_t right);
offset_t insert_into_parent( offset_t root, offset_t left, uint64_t key, offset_t right );
offset_t insert_into_new_root( offset_t left, uint64_t key, offset_t right );
offset_t start_new_tree( uint64_t key, offset_t pointer );
offset_t insert( offset_t root, uint64_t key, int value );

// Deletion.

int get_neighbor_index( node * n );
node * adjust_root(node * root);
node * coalesce_nodes(node * root, node * n, node * neighbor,
                      int neighbor_index, int k_prime);
node * redistribute_nodes(node * root, node * n, node * neighbor,
                          int neighbor_index,
        int k_prime_index, int k_prime);
node * delete_entry( node * root, node * n, uint64_t key, void * pointer );
node * delete( node * root, uint64_t key );

void destroy_tree_nodes(node * root);
node * destroy_tree(node * root);

// File Management

// Exit handler
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
int insert (int64_t key, char * value);
/*
    Find the record containing input ‘key’.
    If found matching ‘key’, return matched ‘value’ string. Otherwise, return NULL.
*/
char * find (int64_t key);
/*
    Find the matching record and delete it if found.
    If success, return 0. Otherwise, return non-zero value.
*/
int delete (int64_t key);

#endif /* __BPT_H__*/