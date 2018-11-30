#include "page.h"

#include <cstring>

Page::Page()
{
	clear();
}

// Getters and Setters

// for Header Page

// -> Setters
void Page::setFreePageOffset(offset_t free_page_offset)
{
	this->free_page_offset = free_page_offset;
}

void Page::setRootPageOffset(offset_t root_page_offset)
{
	this->root_page_offset = root_page_offset;
}

void Page::setNumOfPages(pagenum_t num_of_page)
{
	this->num_of_pages = num_of_page;
}

void Page::setNumOfColumns(table_colnum_t num_of_column)
{
	this->num_of_columns = num_of_column;
} 

// for Free Page

// -> Setters
void Page::setNextFreePageOffset(offset_t next_free_page_offset) {
    this->next_free_page_offset = next_free_page_offset;
}

// for Node Page

std::vector<record_val_t> Page::getRecord(key_idx_t index, int num_column)
{
	std::vector<record_val_t> result(num_column);
	
	if ( isLeaf() && index < DEFAULT_LEAF_ORDER - 1 )
	{
		result[0] = records[index].key;
		for ( auto i = 1; i < num_column; ++i )
			result[i] = records[index].values[i - 1];
	}
	return result;
}

// -> Setters
int Page::setParentOffset(offset_t offset) {
    this->offset = offset;
    return SUCCESS;
}
int Page::setLeaf() {
    this->isLeafNode = true;
    return SUCCESS;
}
int Page::setInternal() {
    this->isLeafNode = false;
    return SUCCESS;
}
int Page::setNumOfKeys(key_idx_t num_keys) {
    this->num_keys = num_keys;
    return SUCCESS;
}
int Page::setKey(key_idx_t index, record_key_t key) {
    if (!isLeaf()) {
        // Internal Page : 0 ≤ index < DEFAULT_INTERNAL_ORDER - 1
        if (index >= DEFAULT_INTERNAL_ORDER - 1)
            return INVALID_INDEX;

        this->pairs[index].key = key;
        return SUCCESS;
    } else {
        // Leaf Page : 0 ≤ index < DEFAULT_LEAF_ORDER - 1
        if (index >= DEFAULT_LEAF_ORDER - 1)
            return INVALID_INDEX;

        this->records[index].key = key;
		return SUCCESS;
    }
}
int Page::setOffset(key_idx_t index, offset_t offset) {
    if (!isLeaf()) {
        // Internal Page : 0 ≤ index < DEFAULT_INTERNAL_ORDER
        if (index >= DEFAULT_INTERNAL_ORDER)
            return INVALID_INDEX;
        else if (index == 0)
            this->one_more_page_offset = offset;
        else
            this->pairs[index - 1].offset = offset;
        return SUCCESS;
    } else {
        // Leaf Page : index == DEFAULT_LEAF_ORDER - 1
        if (index == DEFAULT_LEAF_ORDER - 1) {
            this->right_sibling_page_offset = offset;
            return SUCCESS;
        } else {
            return INVALID_INDEX;
        }
    }
}
int Page::setValues(key_idx_t index, const record_val_t* src, int num_column) {
    if (isLeaf()) {
        // Leaf Page : 0 ≤ index < DEFAULT_LEAF_ORDER - 1
        if (index >= DEFAULT_LEAF_ORDER - 1) {
            return INVALID_INDEX;
        }
		if(src==nullptr)
			std::memset(this->records[index].values, 0, (MAX_NUM_COLUMNS - 1) * sizeof(record_val_t));
		else
			std::memcpy(this->records[index].values, src, (num_column - 1) * sizeof(record_val_t));
        return SUCCESS;
    } else {
        return INVALID_PAGE;
    }
}

int Page::binarySearch(record_key_t key) const
{
	int l, r, m, middle_key;
	l = 0;
	r = getNumOfKeys() - 1;
	while ( l <= r )
	{
		m = (l + r) / 2;
		middle_key = getKey(m);
		if ( middle_key < key )
			l = m + 1;
		else if ( middle_key > key )
			r = m - 1;
		else
			return m;
	}
	return INVALID_KEY; /* cannot find the given key in the page */
}

int Page::binaryRangeSearch(record_key_t key) const
{
	/* Modified Version of Binary Search */
	int l, r, m, middle_key;
	l = 0;
	r = getNumOfKeys();
	while ( l < r )
	{
		m = (l + r) / 2;

		if ( l == m )
			return m;

		middle_key = getKey(m);
		if ( middle_key <= key )
			l = m;
		else
			r = m;
	}
	return INVALID_KEY; /* No Key is provided. */
}

void Page::clear()
{
	memset(page, 0, PAGESIZE);
}

void* Page::operator&()
{
	return &page;
}

const void* Page::operator&() const
{
	return const_cast<Page *>(this)->operator&();
}