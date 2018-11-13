#include "disk_manager.h"

#include "buffer_manager.h"
#include "page_access_manager.h"
#include "macros.h"

#include <stdio.h> /* perror */
#include <stdlib.h> /* atexit */
#include <fcntl.h> /* file control */
#include <unistd.h> /* open, close, lseek */

#define READ(tid, buf) (read(fds[(tid)-1], (buf), PAGESIZE))
#define WRITE(tid, buf) (write(fds[(tid)-1], (buf), PAGESIZE))
#define CLOSE(tid) (close(fds[tid-1]))
#define SEEK(tid, offset) (lseek(fds[(tid)-1], (offset), SEEK_SET) >= 0)
#define CLEAR(buf) (memset((buf), 0, PAGESIZE))
#define FD(tid) *(&fds[(tid)-1])
#define IS_TID_OPEN(tid) (fds[(tid)-1] != 0)

/*
 * For automatic DB shutdown
 */
static bool initialized = false;

/*
 * File Descriptor for R/W
 */
static int fds[DEFAULT_SIZE_OF_TABLES];

/*
 *  Read an on-disk page into the in-memory page structure(dest)
 */
void file_read_page(int table_id, pagenum_t pagenum, page_t* dest){
	if(!(IS_VALID_TID(table_id) && IS_TID_OPEN(table_id)))
        return;

	SEEK(table_id, OFFSET(pagenum));
	READ(table_id, dest);
}

/*
 *  Write an in-memory page(src) to the on-disk page
 */
void file_write_page(int table_id, pagenum_t pagenum, const page_t* src){
	if(!(IS_VALID_TID(table_id) && IS_TID_OPEN(table_id)))
        return;

	SEEK(table_id, OFFSET(pagenum));
	WRITE(table_id, src);
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
    buffer_frame_t *buf_header, *buf_free_pg;
    page_t *header, *free_page;

    buf_header = buf_get_frame(table_id, HEADER_PAGE_NUM);
    header = buf_get_page(buf_header, true);

    if (getFreePageOffset(header) == HEADER_PAGE_OFFSET) {
        page_t buf;
        CLEAR(&buf);
        int i;
        pagenum_t next_free_page_num = getNumOfPages(header);

        setFreePageOffset(header, OFFSET(next_free_page_num));
        for (i = 1; i <= DEFAULT_SIZE_OF_FREE_PAGES; ++i, ++next_free_page_num) {
            setNextFreePageOffset(&buf, i != DEFAULT_SIZE_OF_FREE_PAGES ? OFFSET(next_free_page_num + 1) : HEADER_PAGE_OFFSET);
            file_write_page(table_id, next_free_page_num, &buf);
        }

        // Set Number of Pages
        setNumOfPages(header, getNumOfPages(header) + DEFAULT_SIZE_OF_FREE_PAGES);
    }
    // Get a free page offset from header page.
    offset_t free_page_offset = getFreePageOffset(header);

    // Read the free page.
    buf_free_pg = buf_get_frame(table_id, PGNUM(free_page_offset)); // 여기서 널포인터가 반환됨.
    free_page = buf_get_page(buf_free_pg, false);

    // Get a next free page offset from the free page.
    setFreePageOffset(header, getNextFreePageOffset(free_page));
    
    buf_put_frame(buf_free_pg);
    buf_put_frame(buf_header);
    return free_page_offset;
}

/*
 *  Free an on-disk page to the free page list
 */
void file_free_page(int table_id, pagenum_t pagenum) {
    if(!(IS_VALID_TID(table_id) && IS_TID_OPEN(table_id)))
        return;

    buffer_frame_t *buf_header, *buf_free_pg;
    page_t *header, *page;

    // Read the given page and header page.
    buf_header = buf_get_frame(table_id, HEADER_PAGE_NUM);
    buf_free_pg = buf_get_frame(table_id, pagenum);

    header = buf_get_page(buf_header, true);
    page = buf_get_page(buf_free_pg, true);

    // Clear
    CLEAR(page);

    // Add it into the free page list
    setNextFreePageOffset(page, getNextFreePageOffset(header));
    setNextFreePageOffset(header, OFFSET(pagenum));

    // Apply changes to the header page.
    buf_flush_frame(buf_header);
    buf_put_frame(buf_header);

    // Free it
    buf_close_frame(buf_free_pg);
    buf_put_frame(buf_free_pg);
}

/*
 * Open existing data file using ‘pathname’ or create one if not existed.
 * If success, return table_id.
 */
int file_open_table(char * pathname){
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
        atexit(buf_shutdown);

        initialized = true;
    }

    if ( FD(tid) == -1) {
        perror("open_db error");
        return INVALID_FD;
    }

    page_t header;

    // Read the header from the file
    if (READ(tid, &header) == 0) {
        // If the header page doesn't exist, create a new one.
        CLEAR(&header);
        setNumOfPages(&header, 1); // the number of created pages
        fsync(FD(tid));
        WRITE(tid, &header);        
    }

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
    buf_close_table(table_id);    
    CLOSE(table_id);
    FD(table_id) = 0;
    
    return SUCCESS;
}