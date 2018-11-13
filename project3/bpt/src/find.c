#include "index_and_file_manager.h"

#include "macros.h"
#include "buffer_manager.h"
#include "page_access_manager.h"

#include <stdlib.h>

offset_t find_leaf( int table_id, offset_t root, uint64_t key ) {
    offset_t c = root;
    buffer_frame_t* buf = NULL;
    page_t* page;    

    while (c != HEADER_PAGE_OFFSET) {
        buf = buf_get_frame(table_id, PGNUM(c));
        assert(buf != NULL);
        page = buf_get_page(buf, false);
        // If the page is leaf,
        if (isLeaf(page))
            break; // break the loop.

        // If the page is internal,
        /* If the given key is smaller than the least key in the page */
        if (key < getKey(page, 0)) {
            // Set the search offset to be the one-more-page-leaf-offset.
            c = getOffset(page, 0);
        } else {
            int index = binaryRangeSearch(page, key);
            if (index == INVALID_KEY){
                c = HEADER_PAGE_OFFSET;
                break;
            }
            // Right side of the index
            c = getOffset(page, index + 1);
        }
        buf_put_frame(buf);
    }
    buf_put_frame(buf);
    return c;
}

record_t * find_record( int table_id, offset_t root, uint64_t key ) {
    offset_t c = find_leaf( table_id, root, key );
    record_t* ret = NULL;

    if (c == HEADER_PAGE_OFFSET) {
        return ret;
    }

    buffer_frame_t* buf = NULL;
    buf = buf_get_frame(table_id, PGNUM(c));
    assert(buf != NULL);
    page_t* leaf_page = buf_get_page(buf, false);

    int index = binarySearch(leaf_page, key);

    if (index == INVALID_KEY)
        goto error;

    if (!(ret = malloc(DEFAULT_RECORD_SIZE)))
        goto error;

    ret->key = getKey(leaf_page, index);
    getValue(leaf_page, index, ret->value);
    
error:
    buf_put_frame(buf);
    return ret;
}