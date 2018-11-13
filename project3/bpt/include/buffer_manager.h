#ifndef __BUFFER_MANAGER_H__
#define __BUFFER_MANAGER_H__

#define PAGE_PTR(buf) &buf->frame

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
 *  Get the page and set whether this block gets dirty.
 */
page_t* buf_get_page(buffer_frame_t* frame, bool dirty);

/*
 *  Flush the buffers of the given table and clear them.
 */
void buf_close_table(int table_id);

/*
 *  Flush the specific buffer and clear it.
 */
void buf_close_frame(buffer_frame_t* frame);

/*
 *  Flush the buffers of the given table.
 */
void buf_flush_table(int table_id);

/*
 *  Flush the specific buffer.
 */
void buf_flush_frame(buffer_frame_t* frame);

/*
 *  Free it from the disk and the buffer.
 */
void buf_free_frame(buffer_frame_t* frame);

/*
 *  Flush all buffers in the buffer pool and free the pool.
 */
void buf_shutdown(void);

#endif