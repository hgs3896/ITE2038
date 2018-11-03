#include "file_manager.h"

#include "page_access_manager.h"
#include "insert.h"
#include "delete.h"

#include <stdio.h>
#include <stdlib.h> /* malloc, free, atexit */
#include <string.h>
#include <fcntl.h> /* file control */
#include <sys/stat.h> /* system constants */
#include <unistd.h> /* open, close */

/*
 *  Find the record containing input ‘key’.
 *  If found matching ‘key’, return matched ‘value’ string. Otherwise, return NULL.
 */
char * find (keynum_t key) {
    record_t* record;

    file_read_page(HEADER_PAGE_NUM, &header);

    if ((record = find_record(getRootPageOffset(&header), key)) == NULL)
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
int insert (keynum_t key, char * value) {
    record_t record;

    // Set the data
    record.key = key;
    strncpy(record.value, value, 120);

    file_read_page(HEADER_PAGE_NUM, &header);

    // Insert the record
    offset_t root_offset = insert_record(getRootPageOffset(&header), &record);

    // Update the root offset
    file_read_page(HEADER_PAGE_NUM, &header);
    setRootPageOffset(&header, root_offset);
    file_write_page(HEADER_PAGE_NUM, &header);

    return root_offset == KEY_EXIST;
}

/*
 *  Allocate an on-disk page from the free page list
 */
pagenum_t file_alloc_page() {

    /*
     * Only if a file doesn't have enough free pages,
     * add free pages into the free page list
     * with an amount of DEFAULT_SIZE_OF_FREE_PAGES.
     */

    /* Update the cached header page. */
    file_read_page(HEADER_PAGE_NUM, &header);

    page_t buf;

    if (getFreePageOffset(&header) == HEADER_PAGE_OFFSET) {
        CLEAR(buf);
        int i;
        pagenum_t next_free_page_num = getNumOfPages(&header);
        setFreePageOffset(&header, OFFSET(next_free_page_num));
        for (i = 1; i <= DEFAULT_SIZE_OF_FREE_PAGES; ++i, ++next_free_page_num) {
            setNextFreePageOffset(&buf, i != DEFAULT_SIZE_OF_FREE_PAGES ? OFFSET(next_free_page_num + 1) : HEADER_PAGE_OFFSET);
            file_write_page(next_free_page_num, &buf);
        }
        // Set Number of Pages
        setNumOfPages(&header, getNumOfPages(&header) + DEFAULT_SIZE_OF_FREE_PAGES);
        // Sync the cached header page with the file.
        file_write_page(HEADER_PAGE_NUM, &header);
    }
    // Get a free page offset from header page.
    offset_t free_page_offset = getFreePageOffset(&header);
    // Read the free page.
    file_read_page(PGNUM(free_page_offset), &buf);
    // Get a next free page offset from the free page.
    setFreePageOffset(&header, getNextFreePageOffset(&buf));
    // Wrtie the header page and sync the cached header page with the file.
    file_write_page(HEADER_PAGE_NUM, &header);

    return free_page_offset;
}

/*
 *  Free an on-disk page to the free page list
 */
void file_free_page(pagenum_t pagenum) {
    page_t buf;

    // Read the given page and header page.
    file_read_page(pagenum, &buf);
    file_read_page(HEADER_PAGE_NUM, &header);

    // Clear
    CLEAR(buf);

    // Add it into the free page list
    setNextFreePageOffset(&buf, getNextFreePageOffset(&header));
    setNextFreePageOffset(&header, OFFSET(pagenum));

    // Apply changes to the header page.
    file_write_page(HEADER_PAGE_NUM, &header);
    file_write_page(pagenum, &buf);
}

/*
 *  Read an on-disk page into the in-memory page structure(dest)
 */
void file_read_page(pagenum_t pagenum, page_t* dest) {
    SEEK(OFFSET(pagenum));
    READ(*dest);
}

/*
 *  Write an in-memory page(src) to the on-disk page
 */
void file_write_page(pagenum_t pagenum, const page_t* src) {
    SEEK(OFFSET(pagenum));
    WRITE(*src);
}

/*
 *  Open existing data file using ‘pathname’ or create one if not existed.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int open_db (char *pathname) {
    if (pathname == NULL)
        return INVALID_FILENAME;

    if (fd != 0)
        return INVALID_FD; // FD is already open!

    // Make a file if it does not exist.
    // Do not use a symbolic link.
    // Flush the buffer whenever try to write.
    // Permission is set to 0644.
    fd = open(pathname, O_CREAT | O_RDWR | O_NOFOLLOW | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );

    // Add a handler which is to be executed before the process exits.
    atexit(when_exit);

    if (fd == -1) {
        perror("open_db error");
        return INVALID_FD;
    }

    // Read the header from the file
    if (READ(header) == 0) {
        // If the header page doesn't exist, create a new one.
        CLEAR(header);
        setNumOfPages(&header, 1); // the number of created pages
        WRITE(header);
    }

    return SUCCESS;
}

/*
 *  Find the matching record and delete it if found.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int delete(keynum_t key){
    file_read_page(HEADER_PAGE_NUM, &header);

    // Insert the record
    offset_t root_offset = delete_record( getRootPageOffset(&header), key );

    if(root_offset != KEY_EXIST){
        // Update the root offset
        file_read_page(HEADER_PAGE_NUM, &header);

        // Reset the Root Page Offset.
        setRootPageOffset(&header, root_offset);

        file_write_page(HEADER_PAGE_NUM, &header);
        return SUCCESS;
    }
    return -1;    
}

/*
 *  Flush and Sync the buffer and exit.
 */
void when_exit(void) {
    fsync(fd);
    if (fd != 0)
        close(fd);
}