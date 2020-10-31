#ifndef __BUFFER_MANAGER_H__
#define __BUFFER_MANAGER_H__

#include "common.h"
#include "page.h"

class BufferBlock
{
	friend class BufferManager;

private:
	/* • Physical frame: containing up to date contents of target page. */
	Page frame;
	/* • Table id: the unique id of table (per file) */
	int table_id;
	/* • Page number: the target page number within a file. */
	pagenum_t pgnum;
	/* • Is dirty: whether this buffer block is dirty or not. */
	bool dirty;
	/* • Is pinned: whether this buffer is accessed right now. */
	int pin_cnt;
	/* • LRU list next (prev) : buffer blocks are managed by LRU list. */
	BufferBlock *next, *prev;
	/* • Other information can be added with your own buffer manager design. */

	BufferBlock(int table_id = 0, pagenum_t pgnum = 0);
public:
	BufferBlock(const BufferBlock&) = delete;
	int getTableID()
	{
		return table_id;
	};
	pagenum_t getPageNum()
	{
		return pgnum;
	};
	bool isDirty()
	{
		return dirty;
	};
	int countPins()
	{
		return pin_cnt;
	};
	Page& getPage();
};

class BufferManager
{
private:
	static bool initialized;
	static BufferBlock* pool;

	BufferManager() = delete;

	static void putAtFront(BufferBlock* p);

public:
	/*
	 *  Initialize the buffer pool with the given number.
	 */
	static int init(int buf_num);

	/*
	 *  Get the buffer control block from the buffer pool.
	 */
	static BufferBlock* get_frame(int table_id, pagenum_t pagenum);

	/*
	 *  Put the buffer block back to the buffer pool.
	 */
	static void put_frame(BufferBlock* src);

	/*
	 *  Get the page and set whether this block gets dirty.
	 */
	static Page* get_page(BufferBlock* frame, bool dirty);

	/*
	 *  Request the allocatioin of a page to the disk manager
	 *  and return the offset of the new page.
	 */
	static pagenum_t buf_alloc_page(int table_id);

	/*
	 *  Request the unused page to be free to the disk manager.
	 */
	static void buf_free_page(int table_id, pagenum_t page_to_be_free);

	/*
	 *  Flush the buffers of the given table and clear them.
	 */
	static void close_table(int table_id);

	/*
	 *  Flush the specific buffer and clear it.
	 */
	static void close_frame(BufferBlock* frame);

	/*
	 *  Flush the buffers of the given table.
	 */
	static void flush_table(int table_id);

	/*
	 *  Flush the specific buffer.
	 */
	static void flush_frame(BufferBlock* frame);

	/*
	 *  Free it from the disk and the buffer.
	 */
	// static void free_frame(BufferBlock* frame) = delete;

	/*
	 *  Flush all buffers in the buffer pool and free the pool.
	 */
	static void shutdown(void);
};

#endif