#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

#include "types.h"

#include <string.h> /* memset */
#include <stdlib.h> /* malloc, free, atexit */
#include <fcntl.h> /* file control */
#include <sys/stat.h> /* system constants */
#include <unistd.h> /* open, close, lseek */

// MACROs for convinience
#define READ(fd, buf) (read((fd), (buf), PAGESIZE))
#define WRITE(fd, buf) (write((fd), (buf), PAGESIZE))
#define COPY(dest, src) (memcpy((dest), (src), PAGESIZE))
#define CLEAR(buf) (memset((buf), 0, PAGESIZE))
#define SEEK(fd, offset) (lseek((fd), (offset), SEEK_SET) >= 0)
// #define PAGE_ALLOC() (malloc(PAGE_SIZE))
// #define PAGE_FREE(buf_addr) (free(buf_addr))
#define OFFSET(pagenum) ((pagenum) * PAGESIZE)
#define PGNUM(pageoffset) ((pageoffset) / PAGESIZE)

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

// File Manager APIs

/*
 * Initialize buffer pool with given number and buffer manager.
 */
int init_db (int buf_num);

/*
 * Open existing data file using ‘pathname’ or create one if not existed.
 * If success, return table_id.
 */
int open_table (char * pathname);

/*
 * Write the pages relating to this table to disk and close the table.
 */
int close_table(int table_id);

/*
 * Destroy buffer manager.
 */
int shutdown_db(void);

/*
    Find the record containing input ‘key’.
    If found matching ‘key’, return matched ‘value’ string. Otherwise, return NULL.
*/
char* find (int table_id, keynum_t key);

/*
 *  Insert input ‘key/value’ (record) to data file at the right place.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int insert (int table_id, keynum_t key, char * value);

/*
 *  Find the matching record and delete it if found.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int delete (int table_id, keynum_t key);

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
 *  Flush and Sync the buffer and exit.
 */

#endif