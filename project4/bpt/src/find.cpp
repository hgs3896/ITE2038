#include "index_and_file_manager.h"

#include "macros.h"
#include "wrapper_funcs.h"
#include "buffer_manager.h"

#include <stdlib.h>

offset_t find_leaf( int table_id, offset_t root, record_key_t key ) {
    offset_t c = root;
    BufferBlock* buf = NULL;

    while (c != HEADER_PAGE_OFFSET) {
        buf = BufferManager::get_frame(table_id, PGNUM(c));
        assert(buf != NULL);
        auto page = BufferManager::get_page(buf, false);
        // If the page is leaf,
        if (page->isLeaf())
            break; // break the loop.

        // If the page is internal,
        /* If the given key is smaller than the least key in the page */
        if (key < page->getKey(0)) {
            // Set the search offset to be the one-more-page-leaf-offset.
            c = page->getOffset(0);
        } else {
            int index = page->binaryRangeSearch(key);
            if (index == INVALID_KEY){
                c = HEADER_PAGE_OFFSET;
                break;
            }
            // Right side of the index
            c = page->getOffset(index + 1);
        }
        BufferManager::put_frame(buf);
    }
    BufferManager::put_frame(buf);
    return c;
}

record_t * find_record( int table_id, offset_t root, record_key_t key) {
    offset_t c = find_leaf( table_id, root, key );
    record_t* ret = NULL;

    if (c == HEADER_PAGE_OFFSET) {
        return ret;
    }

    BufferBlock* buf = NULL;
    buf = BufferManager::get_frame(table_id, PGNUM(c));
    assert(buf != NULL);
    Page* leaf_page = BufferManager::get_page(buf, false);
	
    auto index = leaf_page->binarySearch(key);

    if (index == INVALID_KEY)
        goto error;

    if (!(ret = new record_t))
        goto error;

    ret->key = leaf_page->getKey(index);
	leaf_page->getValues(index, ret->values, getNumOfCols(table_id));
    
error:
    BufferManager::put_frame(buf);
    return ret;
}