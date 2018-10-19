#include "bpt.h"

// Setters and Getters

// for Header Page
void setFreePageOffset(page_t *page, offset_t free_page_offset) {
	((header_page_t*)page)->free_page_offset = free_page_offset;
}

void setRootPageOffset(page_t *page, offset_t root_page_offset) {
	((header_page_t*)page)->root_page_offset = root_page_offset;
}

void setNumberOfPages(page_t *page, pagenum_t num_of_page) {
	((header_page_t*)page)->num_of_page = num_of_page;
}

offset_t getFreePageOffset(const page_t *page) {
	return ((header_page_t*)page)->free_page_offset;
}

offset_t getRootPageOffset(const page_t *page) {
	return ((header_page_t*)page)->root_page_offset;
}

pagenum_t getNumberOfPages(const page_t *page) {
	return ((header_page_t*)page)->num_of_page;
}

// for Free Page
void setNextFreePageOffset(page_t *page, offset_t next_free_page_offset) {
	((free_page_t*)page)->next_free_page_offset = next_free_page_offset;
}

offset_t getNextFreePageOffset(const page_t *page) {
	return ((free_page_t*)page)->next_free_page_offset;
}

// for Node Page
void setParent(page_t *page, offset_t offset) {
	((node_page_t*)page)->offset = offset;
}

void setLeaf(page_t *page, uint32_t isLeaf) {
	((node_page_t*)page)->isLeaf = isLeaf;
}

void setNumberOfKeys(page_t *page, uint32_t num_keys) {
	((node_page_t*)page)->num_keys = num_keys;
}

void setKey(page_t *page, uint32_t index, uint64_t key) {
	if (index < 0 || index >= getNumberOfKeys(page)) return;
	((node_page_t*)page)->data[index].key = key;
}

void setOffset(page_t *page, uint32_t index, uint64_t offset) {
	if (index < 0 || index >= getNumberOfKeys(page)) return;
	((node_page_t*)page)->data[index].offset = offset;
}

void setRecord(page_t *page, const record_t* record){
	if(record == NULL) return;
	int l = 0, r = ((node_page_t*)page)->num_keys, m;
	while(l<=r){
		m = (l+r)/2
		if(((node_page_t*)page)->record[m].key > record->key){
			r = m - 1;
		}else if(((node_page_t*)page)->record[m].key < record->key){
			l = m + 1;
		}else{
			((node_page_t*)page)->record[m] = *record;
			return;
		}
	}
}

offset_t getParent(const page_t *page) {
	return ((node_page_t*)page)->offset;
}

uint32_t isLeaf(const page_t *page) {
	return ((node_page_t*)page)->isLeaf;
}

uint32_t getNumberOfKeys(const page_t *page) {
	return ((node_page_t*)page)->num_keys;
}

uint64_t getKey(const page_t *page, uint32_t index) {
	if (index < 0 || index >= getNumberOfKeys(page)) return -1;
	return ((node_page_t*)page)->pairs[index].key;
}

uint64_t getOffset(const page_t *page, uint32_t index) {
	if (index < 0 || index > getNumberOfKeys(page)) return -1;
	if (index == 0) return ((node_page_t*)page)->one_more_page_offset;
	else return ((node_page_t*)page)->pairs[index-1].offset;
}

record_t* getRecord(page_t *page, uint64_t key){
	if(record == NULL) return NULL;
	int l = 0, r = ((node_page_t*)page)->num_keys, m;
	while(l<=r){
		m = (l+r)/2
		if(((node_page_t*)page)->record[m].key > key){
			r = m - 1;
		}else if(((node_page_t*)page)->record[m].key < key){
			l = m + 1;
		}else{
			record_t * ret = malloc(sizeof record_t);
			memcpy(ret, ((node_page_t*)page)->record[m]);
			return ret;
		}
	}
	return NULL;
}

void moveRecord(page_t *page, int from, int to){
	copyRecord(((node_page_t*)page)->records[to], ((node_page_t*)page)->records[from]);
}

void copyRecord(record_t* dest, const record_t* src){
	memcpy(dest, src, sizeof(record_t));	
}