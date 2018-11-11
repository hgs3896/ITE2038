#include "buffer_manager.h"

#include <string.h> /* memset */
#include <stdlib.h> /* malloc, free, atexit */
#include <fcntl.h> /* file control */
#include <sys/stat.h> /* system constants */
#include <unistd.h> /* open, close, lseek */

// MACROs for convinience
#define READ(tid, buf) (read(fds[(tid)-1], (buf), PAGESIZE))
#define WRITE(tid, buf) (write(fds[(tid)-1], (buf), PAGESIZE))
#define COPY(dest, src) (memcpy((dest), (src), PAGESIZE))
#define CLEAR(buf) (memset((buf), 0, PAGESIZE))
#define SEEK(tid, offset) (lseek(fds[(tid)-1], (offset), SEEK_SET) >= 0)

#define FRAME_ALLOC() (malloc(sizeof(buffer_frame_t)))
#define FRAME_FREE(buf_addr) (free(buf_addr))

#define IS_VALID_TID(tid) ((tid) >= 1 && (tid) <= DEFAULT_SIZE_OF_TABLES && fds[(tid)-1] != 0)

static bool initialized = false;
static buffer_frame_t* pool = NULL;

/*
 *  Initialize the buffer pool with the given number.
 *  
 *  - Initialize other fields (state info, LRU info..) with your own design.
 *  - If success, return 0.
 *  - Otherwise, return non-zero value.
 */
int buf_init(int buf_num){
	// Initialize the previous buffer pool.
	buf_free();

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
		if(front == NULL){
			front = temp;
		}

		// Allocate new buffer frames into the buffer pool.
		temp->table_id = 0; // 0 means no table id
		temp->pgnum = -1; // initially no page
		temp->dirty = false; // initially clean
		temp->pin_cnt = 0; // initially not used

		temp->prev = pool;
		temp->prev->next = temp;
		pool = temp;
	}

	temp->next = front;
	front->prev = temp;

	// Initialization is finished.
	if(!initialized){
		initialized = true;
	}

	return 0;
}

/*
 *  Get the buffer control block from the buffer pool.
 */
buffer_frame_t* dest buf_get_frame(int table_id, pagenum_t pagenum){
	if(!initialized)
		return;

	buffer_frame_t* p;

	p = pool->next;
	
	/* 
	 * If there is the requested page on the buffer, 
	 * return it.
	 */
	while(p != NULL){
		if(p->table_id == table_id && p->pgnum == pagenum){
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
		if(p->pin_cnt == 0)
			break;
		p = p->prev;
		if(p==pool)
			return NULL; // No frame can be evicted.
	}

	/* Pinned */
	++p->pin_cnt;

	// If the page is dirty,
	if(p->dirty){
		// Flush it and make it clean
		file_write_page(p->table_id, p->pgnum, p->frame);
		p->dirty = false;
	}

	// Replace this page

	// Refill the page metadata
	p->table_id = table_id;
	p->pgnum = pagenum;

	// Read the page
	file_read_page(p->table_id, p->pgnum, p->frame);

	return p;
}

/*
 *  Put the buffer block back to the buffer pool.
 */
void buf_put_frame(buffer_frame_t* src) {
	// If the buffer pool is not initialized,
	if(!initialized)
		return; // return.

	buffer_frame_t* p = pool;

	while(true){
		p = p->next;
		if(p == src)
			break;
		if(p == pool)
			return;
	}

	/* Put it at the front of the list. */
	p->prev->next = p->next;
	p->next->prev = p->prev;
	p->prev = pool;
	p->next = pool->next;
	p->prev->next = p;
	p->next->prev = p;

	/* Unpinned */
	--p->pin_cnt;
}

/*
 *  Flush and Sync the buffer and exit.
 */
void buf_flush (int table_id){
	if(IS_TABLE){

	}
}

/*
 *  Free an buffer block and link it to the free page list.
 */
int buf_free(void){
	if(pool){
		buffer_frame_t* it, *next;
		
		it = pool;
		next = it->next;

		while(next != pool){
			FRAME_FREE(it);
			it = next;
			next = it->next;
		}

		pool = NULL;
	}
}

/*
 *  Read an on-disk page into the in-memory page structure(dest)
 */
void file_read_page(int table_id, pagenum_t pagenum, page_t* dest){
	if(IS_VALID_TID(table_id)){
		SEEK(table_id, OFFSET(pagenum));
	    READ(table_id, dest);
    }
}

/*
 *  Write an in-memory page(src) to the on-disk page
 */
void file_write_page(int table_id, pagenum_t pagenum, const page_t* src){
	if(IS_VALID_TID(table_id)){
		SEEK(table_id, OFFSET(pagenum));
	    WRITE(table_id, src);
	}
}