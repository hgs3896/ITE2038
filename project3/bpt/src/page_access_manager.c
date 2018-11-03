#include "page_access_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// Getters and Setters

// for Header Page
// -> Getters
offset_t getFreePageOffset(const page_t *page) {
    return ((const header_page_t*)page)->free_page_offset;
}
offset_t getRootPageOffset(const page_t *page) {
    return ((const header_page_t*)page)->root_page_offset;
}
pagenum_t getNumOfPages(const page_t *page) {
    return ((const header_page_t*)page)->num_of_page;
}
// -> Setters
int setFreePageOffset(page_t *page, offset_t free_page_offset) {
    ((header_page_t*)page)->free_page_offset = free_page_offset;
    return SUCCESS;
}
int setRootPageOffset(page_t *page, offset_t root_page_offset) {
    ((header_page_t*)page)->root_page_offset = root_page_offset;
    return SUCCESS;
}
int setNumOfPages(page_t *page, pagenum_t num_of_page) {
    ((header_page_t*)page)->num_of_page = num_of_page;
    return SUCCESS;
}

// for Free Page
// -> Getters
offset_t getNextFreePageOffset(const page_t *page) {
    return ((const free_page_t*)page)->next_free_page_offset;
}
// -> Setters
int setNextFreePageOffset(page_t *page, offset_t next_free_page_offset) {
    ((free_page_t*)page)->next_free_page_offset = next_free_page_offset;
    return SUCCESS;
}

// for Node Page
// -> Getters
offset_t getParentOffset(const page_t *page) {
    return ((const node_page_t*)page)->offset;
}
int isLeaf(const page_t *page) {
    return ((const node_page_t*)page)->isLeaf;
}
int getNumOfKeys(const page_t *page) {
    return ((const node_page_t*)page)->num_keys;
}
keynum_t getKey(const page_t *page, uint32_t index) {
    if (!isLeaf(page)) {
        // Internal Page : 0 ≤ index < DEFAULT_INTERNAL_ORDER - 1
        if (index >= DEFAULT_INTERNAL_ORDER - 1) {
            return INVALID_INDEX;
        } else {
            return ((const node_page_t*)page)->pairs[index].key;
        }
    } else {
        // Leaf Page : 0 ≤ index < DEFAULT_LEAF_ORDER - 1
        if (index >= DEFAULT_LEAF_ORDER - 1)
            return INVALID_INDEX;
        else
            return ((const node_page_t*)page)->records[index].key;
    }
}
offset_t getOffset(const page_t *page, uint32_t index) {
    if (!isLeaf(page)) {
        // Internal Page : 0 ≤ index < DEFAULT_INTERNAL_ORDER
        if (index >= DEFAULT_INTERNAL_ORDER | index < 0)
            return INVALID_INDEX;
        else if (index == 0)
            return ((const node_page_t*)page)->one_more_page_offset;
        else
            return ((const node_page_t*)page)->pairs[index - 1].offset;
    } else {
        // Leaf Page : index = DEFAULT_LEAF_ORDER - 1
        if (index == DEFAULT_LEAF_ORDER - 1)
            return ((const node_page_t*)page)->right_sibling_page_offset;
        else
            return INVALID_INDEX;
    }
}
int getValue(const page_t *page, uint32_t index, char *desc) {
    if (isLeaf(page)) {
        // Leaf Page : 0 ≤ index < DEFAULT_LEAF_ORDER - 1
        if (index >= DEFAULT_LEAF_ORDER - 1) {
            return INVALID_INDEX;
        }
        strncpy(desc, ((const node_page_t*)page)->records[index].value, 120);
        return SUCCESS;
    } else {
        return INVALID_PAGE;
    }
}
// -> Setters
int setParentOffset(page_t *page, offset_t offset) {
    ((node_page_t*)page)->offset = offset;
    return SUCCESS;
}
int setLeaf(page_t *page) {
    ((node_page_t*)page)->isLeaf = true;
    return SUCCESS;
}
int setInternal(page_t *page) {
    ((node_page_t*)page)->isLeaf = false;
    return SUCCESS;
}
int setNumOfKeys(page_t *page, uint32_t num_keys) {
    ((node_page_t*)page)->num_keys = num_keys;
    return SUCCESS;
}
int setKey(page_t *page, uint32_t index, uint64_t key) {
    if (!isLeaf(page)) {
        // Internal Page : 0 ≤ index < DEFAULT_INTERNAL_ORDER - 1
        if (index >= DEFAULT_INTERNAL_ORDER - 1)
            return INVALID_INDEX;

        ((node_page_t*)page)->pairs[index].key = key;
        return SUCCESS;
    } else {
        // Leaf Page : 0 ≤ index < DEFAULT_LEAF_ORDER - 1
        if (index >= DEFAULT_LEAF_ORDER - 1)
            return INVALID_INDEX;

        ((node_page_t*)page)->records[index].key = key;
    }
}
int setOffset(page_t *page, uint32_t index, uint64_t offset) {
    if (!isLeaf(page)) {
        // Internal Page : 0 ≤ index < DEFAULT_INTERNAL_ORDER
        if (index >= DEFAULT_INTERNAL_ORDER)
            return INVALID_INDEX;
        else if (index == 0)
            ((node_page_t*)page)->one_more_page_offset = offset;
        else
            ((node_page_t*)page)->pairs[index - 1].offset = offset;
        return SUCCESS;
    } else {
        // Leaf Page : index == DEFAULT_LEAF_ORDER - 1
        if (index == DEFAULT_LEAF_ORDER - 1) {
            ((node_page_t*)page)->right_sibling_page_offset = offset;
            return SUCCESS;
        } else {
            return INVALID_INDEX;
        }
    }
}
int setValue(page_t *page, uint32_t index, const char *src) {
    if (isLeaf(page)) {
        // Leaf Page : 0 ≤ index < DEFAULT_LEAF_ORDER - 1
        if (index >= DEFAULT_LEAF_ORDER - 1) {
            return INVALID_INDEX;
        }

        strncpy(((node_page_t*)page)->records[index].value, src, 120);
        return SUCCESS;
    } else {
        return INVALID_PAGE;
    }
}