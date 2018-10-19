#include "bpt.h"

// Setters and Getters

// for Header Page
void setFreePageOffset(page_t *page, offset_t free_page_offset) {
	page->header_page.free_page_offset = free_page_offset;
}

void setRootPageOffset(page_t *page, offset_t root_page_offset) {
	page->header_page.root_page_offset = root_page_offset;
}

void setNumberOfPages(page_t *page, pagenum_t num_of_page) {
	page->header_page.num_of_page = num_of_page;
}

offset_t getFreePageOffset(const page_t *page) {
	return page->header_page.free_page_offset;
}

offset_t getRootPageOffset(const page_t *page) {
	return page->header_page.root_page_offset;
}

pagenum_t getNumberOfPages(const page_t *page) {
	return page->header_page.num_of_page;
}

// for Free Page
void setNextFreePageOffset(page_t *page, offset_t next_free_page_offset) {
	page->free_page.next_free_page_offset = next_free_page_offset;
}

offset_t getNextFreePageOffset(const page_t *page) {
	return page->free_page.next_free_page_offset;
}

// for Node Page
void setParent(page_t *page, offset_t offset) {
	page->node_page.offset = offset;
}

void setLeaf(page_t *page, uint32_t isLeaf) {
	page->node_page.isLeaf = isLeaf;
}

void setNumberOfKeys(page_t *page, uint32_t num_keys) {
	page->node_page.num_keys = num_keys;
}

void setKey(page_t *page, uint32_t index, uint64_t key) {
	if (index < 0 || index >= getNumberOfKeys(page)) return;
	page->node_page.pairs[index].key = key;
}

void setOffset(page_t *page, uint32_t index, uint64_t offset) {
	if (index < 0 || index >= getNumberOfKeys(page)) return;
	page->node_page.pairs[index].offset = offset;
}

void setRecord(page_t *page, const record_t* record){
	if(record == NULL) return;
	int l = 0, r = page->node_page.num_keys, m;
	while(l<=r){
		m = (l+r)/2;
		if(page->node_page.records[m].key > record->key){
			r = m - 1;
		}else if(page->node_page.records[m].key < record->key){
			l = m + 1;
		}else{
			copyRecord(&page->node_page.records[m], record);
			return;
		}
	}
}

offset_t getParent(const page_t *page) {
	return page->node_page.offset;
}

uint32_t isLeaf(const page_t *page) {
	return page->node_page.isLeaf;
}

uint32_t getNumberOfKeys(const page_t *page) {
	return page->node_page.num_keys;
}

uint64_t getKey(const page_t *page, uint32_t index) {
	if (index < 0 || index >= getNumberOfKeys(page)) return -1;
	return page->node_page.pairs[index].key;
}

uint64_t getOffset(const page_t *page, uint32_t index) {
	if (index < 0 || index > getNumberOfKeys(page)) return -1;
	if (index == 0) return page->node_page.one_more_page_offset;
	else return page->node_page.pairs[index-1].offset;
}

record_t* getRecord(page_t *page, uint64_t key){
	int l = 0, r = page->node_page.num_keys, m;
	while(l<=r){
		m = (l+r)/2;
		if(page->node_page.records[m].key > key){
			r = m - 1;
		}else if(page->node_page.records[m].key < key){
			l = m + 1;
		}else{
			record_t * ret = malloc(sizeof(record_t));
			copyRecord(ret, &page->node_page.records[m]);
			return ret;
		}
	}
	return NULL;
}

void moveRecord(page_t *page, int from, int to){
	copyRecord(&page->node_page.records[to], &page->node_page.records[from]);
}

void copyRecord(record_t* dest, const record_t* src){
	memcpy(dest, src, sizeof(record_t));	
}