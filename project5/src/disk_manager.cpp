#include "disk_manager.h"

#include "wrapper_funcs.h"
#include "buffer_manager.h"
#include "macros.h"

#include <cstdio> /* perror */
#include <cstdlib> /* atexit */
#include <fcntl.h> /* file control */
#include <unistd.h> /* open, close, lseek */
#include <sys/stat.h> /* system constants */

#define READ(tid, buf) (read(fds[(tid)-1], (buf), PAGESIZE))
#define WRITE(tid, buf) (write(fds[(tid)-1], (buf), PAGESIZE))
#define CLOSE(tid) (close(fds[tid-1]))
#define SEEK(tid, offset) (lseek(fds[(tid)-1], (offset), SEEK_SET) >= 0)
#define FD(tid) *(&fds[(tid)-1])
#define NUM_COL(tid) *(&num_cols[(tid)-1])
#define IS_TID_OPEN(tid) (fds[(tid)-1] != 0)

/*
 * For automatic DB shutdown
 */
static bool initialized = false;

/*
 * File Descriptor for R/W
 */
static int fds[DEFAULT_SIZE_OF_TABLES];
static int num_cols[DEFAULT_SIZE_OF_TABLES];

/*
 *  Read an on-disk page into the in-memory page structure(dest)
 */
void file_read_page(int table_id, pagenum_t pagenum, Page& dest){
	if(!(IS_VALID_TID(table_id) && IS_TID_OPEN(table_id)))
        return;

	SEEK(table_id, OFFSET(pagenum));
	READ(table_id, &dest);
}

/*
 *  Write an in-memory page(src) to the on-disk page
 */
void file_write_page(int table_id, pagenum_t pagenum, const Page& src){
	if(!(IS_VALID_TID(table_id) && IS_TID_OPEN(table_id)))
        return;

	SEEK(table_id, OFFSET(pagenum));
	WRITE(table_id, &src);
    fsync(FD(table_id));
}

/*
 *  Allocate an on-disk page from the free page list
 */
pagenum_t file_alloc_page(int table_id) {
    if(!(IS_VALID_TID(table_id) && IS_TID_OPEN(table_id)))
        return INVALID_TID;

	/*
     * Only if a file doesn't have enough free pages,
     * add free pages into the free page list
     * with an amount of DEFAULT_SIZE_OF_FREE_PAGES.
     */

    /* Update the cached header page. */
    auto buf_header = BufferManager::get_frame(table_id, HEADER_PAGE_NUM);
    auto header = BufferManager::get_page(buf_header, true);

    if ( header->getFreePageOffset() == HEADER_PAGE_OFFSET) {
        Page buf;
		buf.clear();
		
		pagenum_t next_free_page_num = header->getNumOfPages();

		header->setFreePageOffset(OFFSET(next_free_page_num));
        for ( int i = 1; i <= DEFAULT_SIZE_OF_FREE_PAGES; ++i, ++next_free_page_num) {
            buf.setNextFreePageOffset(i != DEFAULT_SIZE_OF_FREE_PAGES ? OFFSET(next_free_page_num + 1) : HEADER_PAGE_OFFSET);
            file_write_page(table_id, next_free_page_num, buf);
        }

        // Set Number of Pages
		header->setNumOfPages(header->getNumOfPages() + DEFAULT_SIZE_OF_FREE_PAGES);
    }

    // Get a free page offset from header page.
    offset_t free_page_offset = header->getFreePageOffset();

    // Read the free page.
    auto buf_free_pg = BufferManager::get_frame(table_id, PGNUM(free_page_offset));
	assert(buf_free_pg != nullptr);
    auto free_page = BufferManager::get_page(buf_free_pg, false);

    // Get a next free page offset from the free page.
	header->setFreePageOffset(free_page->getNextFreePageOffset());
    
    BufferManager::put_frame(buf_free_pg);
    BufferManager::put_frame(buf_header);
    return free_page_offset;
}

/*
 *  Free an on-disk page to the free page list
 */
void file_free_page(int table_id, pagenum_t pagenum) {
	if ( !(IS_VALID_TID(table_id) && IS_TID_OPEN(table_id)) )
		return;

	// Read the page to remove and the header page.
	auto buf_header = BufferManager::get_frame(table_id, HEADER_PAGE_NUM);
	auto buf_free_pg = BufferManager::get_frame(table_id, pagenum);

	auto header = BufferManager::get_page(buf_header, true);
	auto free_page = BufferManager::get_page(buf_free_pg, true);

	// Clear
	free_page->clear();

	// Add it into the free page list
	free_page->setNextFreePageOffset(header->getFreePageOffset());
	header->setFreePageOffset(OFFSET(pagenum));

	// Apply changes to the header page.
	BufferManager::put_frame(buf_header);

	// Free it
	BufferManager::close_frame(buf_free_pg);
	BufferManager::put_frame(buf_free_pg);
}

/*
 * Open existing data file using ‘pathname’ or create one if not existed.
 * If success, return table_id.
 */
int file_open_table(char * pathname, int num_column){
	if (pathname == NULL)
        return INVALID_FILENAME;

    // Find an empty slot for file descriptor.
    int tid = 1;
    
    while( IS_TID_OPEN(tid) )
        tid++;

    if(tid > DEFAULT_SIZE_OF_TABLES)    
        return FULL_FD; // FD slots are full.

    // Make a file if it does not exist.
    // Do not allow a symbolic link to open.
    // Flush the buffer whenever try to write.
    // Permission is set to 0644.
    FD(tid) = open(
        pathname,
        O_CREAT | O_RDWR  | O_NOFOLLOW | O_SYNC, // Options
        S_IRUSR | S_IWUSR | S_IRGRP    | S_IROTH // Permissions
    );

    if(!initialized){
        // Add a handler to be executed before the process exits.
        // atexit(BufferManager::shutdown);

        initialized = true;
    }

    if ( FD(tid) == -1) {
        perror("open_db error");
        return INVALID_FD;
    }

    Page header;

    // Read the header from the file
    if (READ(tid, &header) == 0) {
        // If the header page doesn't exist, create a new one.
		header.clear();
        header.setNumOfPages(1); // the number of created pages
		header.setNumOfColumns(num_column); // the number of columns
        fsync(FD(tid));
        WRITE(tid, &header);        
    }

	NUM_COL(tid) = header.getNumOfColumns();

    /* unique table id */
    return tid;
}

/*
 * Write the pages relating to this table to disk and close the table.
 *  - Write all pages of this table from buffer to disk and discard the table id.
 *  - If success, return 0. Otherwise, return non-zero value.
 */
int file_close_table(int table_id){
    if(!(IS_VALID_TID(table_id) && IS_TID_OPEN(table_id)))
        return INVALID_TID;

    // writes out the pages only from those relating to given table_id
    BufferManager::close_table(table_id);    
    CLOSE(table_id);
	NUM_COL(table_id) = 0;
    FD(table_id) = 0;
	return SUCCESS;
}

int getNumOfCols(int table_id) noexcept
{
	return IS_VALID_TID(table_id) && IS_TID_OPEN(table_id) ? NUM_COL(table_id) : 0;
}