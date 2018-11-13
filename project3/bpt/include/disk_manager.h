#ifndef __DISK_MANAGER_H__
#define __DISK_MANAGER_H__

#include "types.h"

#include <string.h> /* memset */
#include <fcntl.h> /* file control */
#include <sys/stat.h> /* system constants */
#include <unistd.h> /* open, close, lseek */

/*
 * Open existing data file using ‘pathname’ or create one if not existed.
 * If success, return table_id.
 */
int file_open_table(char * pathname);

/*
 * Write the pages relating to this table to disk and close the table.
 *  - Write all pages of this table from buffer to disk and discard the table id.
 *  - If success, return 0. Otherwise, return non-zero value.
 */
int file_close_table(int table_id);

/*
 *  Read an on-disk page into the in-memory page structure(dest)
 */
void file_read_page(int table_id, pagenum_t pagenum, page_t* dest);

/*
 *  Write an in-memory page(src) to the on-disk page
 */
void file_write_page(int table_id, pagenum_t pagenum, const page_t* src);

/*
 *  Free an on-disk page to the free page list
 */
void file_free_page(int table_id, pagenum_t pagenum);

/*
 *  Allocate an on-disk page from the free page list
 */
pagenum_t file_alloc_page(int table_id);

#endif