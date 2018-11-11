#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

#include "types.h"

/*
 *  Initialize the buffer pool with the given number.
 */
int buf_init(int buf_num);

/*
 *  Get the buffer control block from the buffer pool.
 */
buffer_frame_t* buf_get_frame(int table_id, pagenum_t pagenum);

/*
 *  Put the buffer block back to the buffer pool.
 */
void buf_put_frame(buffer_frame_t* src);

/*
 *  Flush and Sync the buffer and exit.
 */
void buf_flush(int table_id);

/*
 *  Flush all dirty pages in the buffer pool and free the pool.
 */
int buf_free(void);

/*
 *  Read an on-disk page into the in-memory page structure(dest)
 */
void file_read_page(int table_id, pagenum_t pagenum, page_t* dest);

/*
 *  Write an in-memory page(src) to the on-disk page
 */
void file_write_page(int table_id, pagenum_t pagenum, const page_t* src);

#endif