#include "file_manager.h"

#include "page_access_manager.h"
#include "buffer_manager.h"
#include "index_manager.h"

#include <stdio.h>

/*
 * For automatic DB shutdown
 */
static bool initialized = false;

/*
 * File Descriptor for R/W
 */
static int fds[DEFAULT_SIZE_OF_TABLES];

/*
 * Initialize buffer pool with given number and buffer manager.
 */
int init_db (int buf_num){
    init_buf(buf_num);
}

/*
 * Open existing data file using ‘pathname’ or create one if not existed.
 * If success, return table_id.
 */
int open_table (char * pathname){
    if (pathname == NULL)
        return INVALID_FILENAME;

    // Find an empty slot for file descriptor.
    int idx = 0;
    
    while( fds[idx] != 0 )
        idx++;

    if(idx >= DEFAULT_SIZE_OF_TABLES)    
        return FULL_FD; // FD slots are full.

    // Make a file if it does not exist.
    // Do not allow a symbolic link to open.
    // Flush the buffer whenever try to write.
    // Permission is set to 0644.
    fds[idx] = open(
        pathname,
        O_CREAT | O_RDWR  | O_NOFOLLOW | O_SYNC, // Options
        S_IRUSR | S_IWUSR | S_IRGRP    | S_IROTH // Permissions
    );

    if(!initialized){
        // Add a handler to be executed before the process exits.
        atexit(shutdown_db);

        initialized = true;
    }

    if (fds[idx] == -1) {
        perror("open_db error");
        return INVALID_FD;
    }

    // Read the header from the file
    if (READ(fds[idx], &header) == 0) {
        // If the header page doesn't exist, create a new one.
        CLEAR(&header);
        setNumOfPages(&header, 1); // the number of created pages
        WRITE(fds[idx], &header);
    }

    /* unique table id */
    return idx + 1;
}

/*
 * Write the pages relating to this table to disk and close the table.
 *  - Write all pages of this table from buffer to disk and discard the table id.
 *  - If success, return 0. Otherwise, return non-zero value.
 */
int close_table(int table_id){
    // writes out the pages only from those relating to given table_id
    // writes out all dirty buffer block to disk.
    if(table_id >= 1 && table_id <= DEFAULT_SIZE_OF_TABLES && fds[table_id] != 0){
        // table_id에 해당하는 버퍼에 남은 dirty한 버퍼를 다 flush!
        flush_buf(table_id);

        // fsync(fds[table_id]);

        close(fds[table_id-1]);
        fds[table_id-1] = 0;
        return 0;
    }
    return -1;
}

/*
 * Destroy buffer manager.
 *  - Flush all data from buffer and destroy allocated buffer.
 *  - If success, return 0. Otherwise, return non-zero value.
 */
int shutdown_db(void){
    // writes out all dirty buffer block to disk.
    int i;
    for(i=1;i<=DEFAULT_SIZE_OF_TABLES;++i)
        close_table(i);
}


/*
 *  Find the record containing input ‘key’.
 *  If found matching ‘key’, return matched ‘value’ string. Otherwise, return NULL.
 */
char * find (int table_id, keynum_t key) {
    record_t* record;

    file_read_page(table_id, HEADER_PAGE_NUM, &header);

    if ((record = find_record(table_id, getRootPageOffset(&header), key)) == NULL)
        return NULL;

    char * ret = malloc(120);
    strncpy(ret, record->value, 120);
    free(record);
    return ret;
}

/*
 *  Insert input ‘key/value’ (record) to data file at the right place.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int insert (int table_id, keynum_t key, char * value) {
    record_t record;

    // Set the data
    record.key = key;
    strncpy(record.value, value, 120);

    file_read_page(table_id, HEADER_PAGE_NUM, &header);

    // Insert the record
    offset_t root_offset = insert_record(table_id, getRootPageOffset(&header), &record);

    // Update the root offset
    file_read_page(table_id, HEADER_PAGE_NUM, &header);
    setRootPageOffset(&header, root_offset);
    file_write_page(table_id, HEADER_PAGE_NUM, &header);

    return root_offset == KEY_EXIST;
}

/*
 *  Find the matching record and delete it if found.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int delete(int table_id, keynum_t key){
    file_read_page(table_id, HEADER_PAGE_NUM, &header);

    // Insert the record
    offset_t root_offset = delete_record(table_id, getRootPageOffset(&header), key );

    if(root_offset != KEY_EXIST){
        // Update the root offset
        file_read_page(table_id, HEADER_PAGE_NUM, &header);

        // Reset the Root Page Offset.
        setRootPageOffset(&header, root_offset);

        file_write_page(table_id, HEADER_PAGE_NUM, &header);
        return SUCCESS;
    }
    return -1;    
}

/*
 *  Allocate an on-disk page from the free page list
 */
pagenum_t file_alloc_page(int table_id) {

    /*
     * Only if a file doesn't have enough free pages,
     * add free pages into the free page list
     * with an amount of DEFAULT_SIZE_OF_FREE_PAGES.
     */

    /* Update the cached header page. */
    file_read_page(table_id, HEADER_PAGE_NUM, &header);

    page_t buf;

    if (getFreePageOffset(&header) == HEADER_PAGE_OFFSET) {
        CLEAR(&buf);
        int i;
        pagenum_t next_free_page_num = getNumOfPages(&header);
        setFreePageOffset(&header, OFFSET(next_free_page_num));
        for (i = 1; i <= DEFAULT_SIZE_OF_FREE_PAGES; ++i, ++next_free_page_num) {
            setNextFreePageOffset(&buf, i != DEFAULT_SIZE_OF_FREE_PAGES ? OFFSET(next_free_page_num + 1) : HEADER_PAGE_OFFSET);
            file_write_page(table_id, next_free_page_num, &buf);
        }
        // Set Number of Pages
        setNumOfPages(&header, getNumOfPages(&header) + DEFAULT_SIZE_OF_FREE_PAGES);
        // Sync the cached header page with the file.
        file_write_page(table_id, HEADER_PAGE_NUM, &header);
    }
    // Get a free page offset from header page.
    offset_t free_page_offset = getFreePageOffset(&header);
    // Read the free page.
    file_read_page(table_id, PGNUM(free_page_offset), &buf);
    // Get a next free page offset from the free page.
    setFreePageOffset(&header, getNextFreePageOffset(&buf));
    // Wrtie the header page and sync the cached header page with the file.
    file_write_page(table_id, HEADER_PAGE_NUM, &header);

    return free_page_offset;
}

/*
 *  Free an on-disk page to the free page list
 */
void file_free_page(int table_id, pagenum_t pagenum) {
    page_t buf;

    // Read the given page and header page.
    file_read_page(table_id, pagenum, &buf);
    file_read_page(table_id, HEADER_PAGE_NUM, &header);

    // Clear
    CLEAR(&buf);

    // Add it into the free page list
    setNextFreePageOffset(&buf, getNextFreePageOffset(&header));
    setNextFreePageOffset(&header, OFFSET(pagenum));

    // Apply changes to the header page.
    file_write_page(table_id, HEADER_PAGE_NUM, &header);
    file_write_page(table_id, pagenum, &buf);
}

/*
 *  Read an on-disk page into the in-memory page structure(dest)
 */
void file_read_page(int table_id, pagenum_t pagenum, page_t* dest) {
    if(dest == NULL)
        return;
    if(table_id < 0 || table_id >= DEFAULT_SIZE_OF_TABLES || fds[table_id] == 0)
        return;
    buffered_read_page(table_id, pagenum, dest);
}

/*
 *  Write an in-memory page(src) to the on-disk page
 */
void file_write_page(int table_id, pagenum_t pagenum, const page_t* src) {
    if(src == NULL)
        return;
    if(table_id < 0 || table_id >= DEFAULT_SIZE_OF_TABLES || fds[table_id] == 0)
        return;
    buffered_write_page(fd, pagenum, dest);
}