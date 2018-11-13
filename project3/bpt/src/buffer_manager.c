#include "buffer_manager.h"
#include "disk_manager.h"
#include "macros.h"

#include <stdlib.h> /* malloc, free, atexit */
#include <string.h> /* memset */

// MACROs for convinience
#define FRAME_ALLOC() (malloc(sizeof(buffer_frame_t)))
#define FRAME_FREE(buf_addr) (free(buf_addr))

static bool initialized = false;
static buffer_frame_t* pool = NULL;

static void putAtFront(buffer_frame_t* p){
	/* Put it at the front of the list. */
	if(p == pool){
		pool = pool->prev;
	}else{
		p->prev->next = p->next;
		p->next->prev = p->prev;
		p->prev = pool;
		p->next = pool->next;
		p->prev->next = p;
		p->next->prev = p;
	}
}

/*
 *  Initialize the buffer pool with the given number.
 *  
 *  - Initialize other fields (state info, LRU info..) with your own design.
 *  - If success, return 0.
 *  - Otherwise, return non-zero value.
 */
int buf_init(int buf_num){
	buffer_frame_t* temp, *front = NULL;
	
	if(buf_num < 0)
		return -1;

	// Form a circular, doubly-linked list.
	while(buf_num--){
		temp = FRAME_ALLOC();
		if(temp == NULL){
			// Not Enough Memory
			exit(-1);
		}

		// Allocate new buffer frames into the buffer pool.
		temp->table_id = 0; // 0 means no table id
		temp->pgnum = 0; // initially no page
		temp->dirty = false; // initially clean
		temp->pin_cnt = 0; // initially not used

		if(front == NULL){
			front = temp;
			pool = front;
		}else{
			temp->prev = pool;
			temp->prev->next = temp;
			pool = temp;
		}
	}

	temp->next = front;
	temp->next->prev = temp;

	// Initialization is finished.
	if(!initialized){
		initialized = true;
	}

	return SUCCESS;
}

/*
 *  Get the buffer control block from the buffer pool.
 */
buffer_frame_t* buf_get_frame(int table_id, pagenum_t pagenum) {
	if(!initialized)
		return NULL;

	buffer_frame_t* p = pool->next;
	
	/* 
	 * If there is the requested page on the buffer, return it.
	 */
	while(p != NULL){
		if(p->table_id && p->table_id == table_id && p->pgnum == pagenum){
			/* Pinned */
			++p->pin_cnt;
			return p;
		}
		
		if(p == pool)
			break;

		/* Go to the next one. */
		p = p->next;
	}

	/* Find an unpinned frame that can be used for replacement */
	while(p != NULL){
		if(p->pin_cnt == 0){
			// Find a candidate for replacement
			break;
		}
		p = p->prev;
		if(p==pool){
			// No frames can be evicted.
			return NULL;
		}
	}

	// Evict

	// If the page is dirty,
	buf_flush_frame(p);

	// Refill the page metadata
	p->table_id = table_id;
	p->pgnum = pagenum;

	// Read the page
	file_read_page(p->table_id, p->pgnum, &p->frame);

	/* Pinned */
	++p->pin_cnt;

	return p;
}

/*
 *  Put the buffer block back to the buffer pool.
 */
void buf_put_frame(buffer_frame_t* src) {
	if(!initialized)
		return;

	if(src == NULL)
		return;

	putAtFront(src);

	/* Unpinned */
	--src->pin_cnt;
}

/*
 *  Get the page and set whether this block gets dirty.
 */
page_t* buf_get_page(buffer_frame_t* frame, bool dirty){
	if(!frame)
		return NULL;
	frame->dirty |= dirty;
	return &frame->frame;
}

/*
 *  Flush the buffers of the given table and clear them.
 */
void buf_close_table(int table_id){
	if(!initialized)
		return;

	buffer_frame_t* p = pool;
	while(p != NULL){
		if(p->table_id && p->table_id == table_id){
			// If the page is dirty,
			buf_close_frame(p);
		}

		p = p->prev;
		if(p == pool)
			break;
	}
}

/*
 *  Flush the specific buffer and clear it.
 */
void buf_close_frame(buffer_frame_t* frame){
	if(!initialized)
		return;

	if(frame == NULL)
		return;
	
	buf_flush_frame(frame);

	CLEAR(&frame->frame);
	frame->table_id = 0;
	frame->pgnum = 0;
}

/*
 *  Flush the buffers of the given table.
 */
void buf_flush_table(int table_id){
	if(!initialized)
		return;

	buffer_frame_t* p = pool;
	while(p != NULL){
		if(p->table_id && p->table_id == table_id){
			// If the page is dirty,
			buf_flush_frame(p);
		}

		p = p->prev;
		if(p == pool)
			break;
	}
}

/*
 *  Flush the specific buffer.
 */
void buf_flush_frame(buffer_frame_t* frame){
	if(!initialized)
		return;

	if(frame == NULL)
		return;

	++frame->pin_cnt;

	if(frame->dirty){
		page_t* page;
		page = buf_get_page(frame, false);
		file_write_page(frame->table_id, frame->pgnum, page);
		frame->dirty = false;
	}

	--frame->pin_cnt;
}

/*
 *  Free it from the disk and the buffer.
 */
void buf_free_frame(buffer_frame_t* frame){
	file_free_page(frame->table_id, frame->pgnum);
}

/*
 *  Flush all buffers in the buffer pool and free the pool.
 */
void buf_shutdown(void){
	int i;

	if(pool){
		buffer_frame_t* it, *next;
		
		it = pool;
		next = it->next;

		while(next != pool){
			buf_flush_frame(it);
			FRAME_FREE(it);
			it = next;
			next = it->next;
		}

		pool = NULL;
	}
}