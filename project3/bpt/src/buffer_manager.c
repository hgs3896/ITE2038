#include "file_manager.h"
#include <stdlib.h>

static bool initialized = false;
static buffer_frame_t* pool = NULL;

/*
 * Initialize the buffer pool.by allocating the buffer pool (array) with the given number of entries.
 * Initialize other fields (state info, LRU info..) with your own design.
 * If success, return 0. Otherwise, return non-zero value.
 */
int init_buf(int buf_num){
	free_buf();
	buffer_frame_t* temp, *front = NULL;
	
	if(buf_num < 0)
		return -1;

	// Form a doubly linked list.
	while(buf_num--){
		temp = malloc(sizeof(buffer_frame_t));
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
 *  Read an on-disk page into the in-memory page structure(dest)
 */
void buffered_read_page(int table_id, pagenum_t pagenum, page_t* dest) {
	buffer_frame_t* p;

	if(!initialized)
		return;

	p = pool->next;
	
	/* 
	 * If there is the requested page on the buffer, 
	 * return it.
	 */
	while(p != NULL){
		if(p->table_id == table_id && p->pgnum == pagenum){
			/* Pinned */
			++p->pin_cnt;

			/* Copy the content of a requested frame. */
			COPY(dest, p->frame);

			/* Put it at the front of the list. */
			p->prev->next = p->next;
			p->next->prev = p->prev;
			p->prev = pool;
			p->next = pool->next;
			p->prev->next = p;
			p->next->prev = p;
			return;
		}
		
		if(p == pool)
			break;

		/* Go to the next one. */
		p = p->next;
	}

	/* Find an unpinned frame that can be used for replacement */
	do{
		if(!p->pin_cnt)
			break;
		p = p->prev;
	}while(p != pool);

	/* Pinned */
	++p->pin_cnt;

	// If the page is dirty,
	if(p->dirty){
		// Flush it and make it clean
		SEEK(p->table_id, OFFSET(p->pgnum));
    	WRITE(p->table_id, p->frame);
		p->dirty = false;
	}

	// Replace this page
	p->table_id = table_id;
	p->pgnum = pagenum;

	// Read the page
	SEEK(p->table_id, OFFSET(pagenum));
	READ(p->table_id, p->frame);
	COPY(dest, p->frame);

	// Insert into the front
	p->prev->next = p->next;
	p->next->prev = p->prev;
	p->prev = pool;
	p->next = pool->next;
	p->prev->next = p;
	p->next->prev = p;
}

/*
 *  Write an in-memory page(src) to the on-disk page
 */
void buffered_write_page(int table_id, pagenum_t pagenum, const page_t* src) {
	buffer_frame_t* p, *prev;

	if(!initialized)
		return;

	p = prev = buffers;

	while(p != NULL){
		/* 
		 * If there exists the requested page on the buffer, 
		 * use it.
		 */
		if(p->table_id == table_id && p->pgnum == pagenum){
			/* Wait until the page gets unpinned. */
			while(p->pin_cnt);

			/* Pinned */
			++p->pin_cnt;

			/* Copy the content of a requested frame. */
			COPY(p->frame, src);
			p->dirty = true;

			/* Put it at the front of the list. */
			if(prev != p){
				prev->next = p->next;
				p->next = buffer;
				buffer = p;
			}

			/* Unpinned */
			--p->pin_cnt;

			return;			
		}
		/* Save the previous node. */
		prev = p;
		/* go to next page */
		p = p->next;
	}

	/* Otherwise, no matching page exists. */
	p = prev;

	/* Wait until the least-frequently used page gets unpinned. */
	while(p->pin_cnt);

	/* Pinned */
	++p->pin_cnt;

	// If the page is dirty,
	if(p->dirty){
		// Flush it and make it clean
		SEEK(p->table_id, OFFSET(p->pgnum));
    	WRITE(p->table_id, p->frame);
		p->dirty = false;
	}

	// Replace this page
	p->table_id = table_id;
	p->pgnum = pagenum;

	// Write the page
	SEEK(p->table_id, OFFSET(p->pgnum));
    WRITE(p->table_id, p->frame);
	COPY(dest, p->frame);

	// Insert into the front
	p->next = buffer;
	buffer = p;

	/* Unpinned */
	--p->pin_cnt;
}

void finish_read_page(int table_id, pagenum_t pagenum){

}

void flush_buf (int table_id){
	if(table_id >= 0 && table_id < DEFAULT_SIZE_OF_TABLES && fds[table_id] != 0){

	}
}

int free_buf(void){
	if(pool){
		buffer_frame_t* it, *next;
		
		it = pool;
		next = it->next;

		while(next != pool){
			free(it);
			it = next;
			next = it->next;
		}

		pool = NULL;
	}
}