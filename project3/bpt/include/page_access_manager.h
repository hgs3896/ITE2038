#ifndef __PAGE_ACCESS_MANAGER_H__
#define __PAGE_ACCESS_MANAGER_H__

#include "types.h"

// Getters and Setters

// for Header Page
// -> Getters
offset_t getFreePageOffset(const page_t *page);
offset_t getRootPageOffset(const page_t *page);
pagenum_t getNumOfPages(const page_t *page);
// -> Setters
int setFreePageOffset(page_t *page, offset_t free_page_offset);
int setRootPageOffset(page_t *page, offset_t root_page_offset);
int setNumOfPages(page_t *page, pagenum_t num_of_page);

// for Free Page
// -> Getters
offset_t getNextFreePageOffset(const page_t *page);
// -> Setters
int setNextFreePageOffset(page_t *page, offset_t next_free_page_offset);

// for Node Page
// -> Getters
offset_t getParentOffset(const page_t *page);
int isLeaf(const page_t *page);
int getNumOfKeys(const page_t *page);
keynum_t getKey(const page_t *page, uint32_t index);
offset_t getOffset(const page_t *page, uint32_t index);
int getValue(const page_t *page, uint32_t index, char *desc);
// -> Setters
int setParentOffset(page_t *page, offset_t offset);
int setLeaf(page_t *page);
int setInternal(page_t *page);
int setNumOfKeys(page_t *page, uint32_t num_keys);
int setKey(page_t *page, uint32_t index, keynum_t key);
int setOffset(page_t *page, uint32_t index, offset_t offset);
int setValue(page_t *page, uint32_t index, const char *src);

#endif