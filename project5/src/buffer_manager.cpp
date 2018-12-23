#include "buffer_manager.h"
#include "disk_manager.h"
#include "macros.h"

#include <cstring> /* memset */

// Buffer Block

BufferBlock::BufferBlock(int table_id, pagenum_t pgnum)
	:frame(), table_id(table_id), pgnum(pgnum), dirty(false), pin_cnt(0), lock(PTHREAD_MUTEX_INITIALIZER)
{

}

Page& BufferBlock::getPage()
{
	return frame;
}

// Buffer Management

bool BufferManager::initialized = false;
BufferBlock* BufferManager::pool = nullptr;
latch BufferManager::global_lock;

/*
 *  Initialize the buffer pool with the given number.
 *  
 *  - Initialize other fields (state info, LRU info..) with your own design.
 *  - If success, return 0.
 *  - Otherwise, return non-zero value.
 */
int BufferManager::init(int buf_num){
	BufferBlock* temp, *front = NULL;
	
	if(buf_num < 0)
		return -1;

	global_lock = PTHREAD_MUTEX_INITIALIZER;
	
	// Lock
	pthread_mutex_lock(&BufferManager::global_lock);

	// Form a circular, viz doubly-linked list.
	while(buf_num--){
		temp = new BufferBlock();
		if(temp == NULL){
			// Not Enough Memory
			exit(-1);
		}

		// Allocate new buffer frames into the buffer pool.
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

	pthread_mutex_unlock(&BufferManager::global_lock);

	// Initialization is finished.
	if(!initialized){
		initialized = true;
	}

	return SUCCESS;
}

// It must be protected by the global lock at the caller method.
void BufferManager::putAtFront(BufferBlock* p)
{
	/* Put it at the front of the list. */
	if ( p == pool )
	{
		pool = pool->prev;
	}
	else
	{
		p->prev->next = p->next;
		p->next->prev = p->prev;
		p->prev = pool;
		p->next = pool->next;
		p->prev->next = p;
		p->next->prev = p;
	}
}

/*
 *  Get the buffer control block from the buffer pool.
 */
BufferBlock* BufferManager::get_frame(int table_id, pagenum_t pagenum) {
	if(!initialized)
		return NULL;

	pthread_mutex_lock(&BufferManager::global_lock);
	BufferBlock* p = pool->next;
	
	/* 
	 * If there is the requested page on the buffer, return it.
	 */
	while(p != NULL){
		if(p->table_id && p->table_id == table_id && p->pgnum == pagenum){
			pthread_mutex_lock(&p->lock);

			/* Pinned */
			++p->pin_cnt;

			pthread_mutex_unlock(&p->lock);
			pthread_mutex_unlock(&BufferManager::global_lock);
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
			pthread_mutex_unlock(&BufferManager::global_lock);
			// No frames can be evicted.
			return NULL;
		}
	}

	// Evict

	// If the page is dirty,
	BufferManager::flush_frame(p);

	// Get the page lock
	pthread_mutex_lock(&p->lock);

	// Refill the page metadata
	p->table_id = table_id;
	p->pgnum = pagenum;

	// Read the page
	file_read_page(p->table_id, p->pgnum, p->frame);

	/* Pinned */
	++p->pin_cnt;

	pthread_mutex_unlock(&p->lock);
	pthread_mutex_unlock(&BufferManager::global_lock);
	return p;
}

/*
 *  Put the buffer block back to the buffer pool.
 */
void BufferManager::put_frame(BufferBlock* src) {
	if(!initialized)
		return;

	if(src == NULL)
		return;

	pthread_mutex_lock(&BufferManager::global_lock);
	pthread_mutex_lock(&src->lock);

	putAtFront(src);

	/* Unpinned */
	--src->pin_cnt;
	
	pthread_mutex_unlock(&src->lock);
	pthread_mutex_unlock(&BufferManager::global_lock);	
}

/*
 *  Get the page and set whether this block gets dirty.
 */
Page* BufferManager::get_page(BufferBlock* block, bool dirty){
	if ( !block )
		return nullptr;

	pthread_mutex_lock(&block->lock);

	block->dirty |= dirty;
	
	pthread_mutex_unlock(&block->lock);
	
	return static_cast<Page*>(&block->frame);
}

/*
	 *  Request the allocatioin of a page to the disk manager
	 *  and return the number of the new page.
	 */
pagenum_t BufferManager::buf_alloc_page(int table_id)
{
	return file_alloc_page(table_id);
}

/*
 *  Request the unused page to be free to the disk manager.
 */
void BufferManager::buf_free_page(int table_id, pagenum_t page_to_be_free)
{
	file_free_page(table_id, page_to_be_free);
}

/*
 *  Flush the buffers of the given table and clear them.
 */
void BufferManager::close_table(int table_id){
	if(!initialized)
		return;
	
	pthread_mutex_lock(&BufferManager::global_lock);
	
	BufferBlock* p = pool;
	while(p != NULL){
		pthread_mutex_lock(&p->lock);

		if(p->table_id && p->table_id == table_id){
			// If the page is dirty,
			BufferManager::close_frame(p);
		}

		pthread_mutex_unlock(&p->lock);

		p = p->prev;
		if(p == pool)
			break;
	}

	pthread_mutex_unlock(&BufferManager::global_lock);
}

/*
 *  Flush the specific buffer and clear it.
 */
void BufferManager::close_frame(BufferBlock* frame){
	if(!initialized)
		return;

	if(frame == NULL)
		return;
	
	// Flush the buffer
	BufferManager::flush_frame(frame);

	pthread_mutex_lock(&BufferManager::global_lock);
	pthread_mutex_lock(&frame->lock);

	// Reset the information of the buffer block
	frame->frame.clear();
	frame->table_id = 0;
	frame->pgnum = 0;

	pthread_mutex_unlock(&frame->lock);
	pthread_mutex_unlock(&BufferManager::global_lock);
}

/*
 *  Flush the buffers of the given table.
 */
void BufferManager::flush_table(int table_id){
	if(!initialized)
		return;

	pthread_mutex_lock(&BufferManager::global_lock);
	BufferBlock* p = pool;
	while(p != NULL){
		pthread_mutex_lock(&p->lock);

		if(p->table_id && p->table_id == table_id){
			// If the page is dirty,
			BufferManager::flush_frame(p);
		}

		pthread_mutex_unlock(&p->lock);
	
		p = p->prev;
		if(p == pool)
			break;
	}
	pthread_mutex_unlock(&BufferManager::global_lock);
}

/*
 *  Flush the specific buffer.
 */
void BufferManager::flush_frame(BufferBlock* frame){
	if(!initialized)
		return;

	if(frame == NULL)
		return;
	
	pthread_mutex_lock(&frame->lock);
	
	++frame->pin_cnt;

	if(frame->dirty){
		file_write_page(frame->table_id, frame->pgnum, frame->frame);
		frame->dirty = false;
	}

	--frame->pin_cnt;

	pthread_mutex_unlock(&frame->lock);
}

/*
 *  Flush all buffers in the buffer pool and free the pool.
 */
void BufferManager::shutdown(void)
{
	int i;
	pthread_mutex_lock(&BufferManager::global_lock);
	if ( pool )
	{
		BufferBlock* it, *next;

		it = pool;
		next = it->next;

		while ( next != pool )
		{
			BufferManager::flush_frame(it);
			delete it;
			it = next;
			next = it->next;
		}

		pool = NULL;
	}
	pthread_mutex_unlock(&BufferManager::global_lock);
}