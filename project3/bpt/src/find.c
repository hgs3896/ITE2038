#include "index_manager.h"
#include "file_manager.h"
#include "page_access_manager.h"

offset_t find_leaf( int table_id, offset_t root, uint64_t key ) {
    offset_t c = root;
    page_t buf;

    while (c != HEADER_PAGE_OFFSET) {
        file_read_page(table_id, PGNUM(c), &buf);
        // If the page is leaf,
        if (isLeaf(&buf))
            break; // break the loop.

        // If the page is internal,
        /* If the given key is smaller than the least key in the page */
        if (key < getKey(&buf, 0)) {
            // Set the search offset to be the one-more-page-leaf-offset.
            c = getOffset(&buf, 0);
        } else {
            int index = binaryRangeSearch(&buf, key);
            if (index == INVALID_KEY)
                return HEADER_PAGE_OFFSET;
            // Right side of the index
            c = getOffset(&buf, index + 1);
        }
    }
    return c;
}

record_t * find_record( int table_id, offset_t root, uint64_t key ) {
    offset_t c = find_leaf( table_id, root, key );

    if (c == HEADER_PAGE_OFFSET) {
        return NULL;
    }

    page_t leaf_page;
    file_read_page(table_id, PGNUM(c), &leaf_page);

    int index = binarySearch(&leaf_page, key);

    if (index == INVALID_KEY)
        return NULL;

    record_t* ret = malloc( sizeof(record_t) );
    if (!ret)
        return NULL;

    ret->key = getKey(&leaf_page, index);
    getValue(&leaf_page, index, ret->value);
    return ret;
}