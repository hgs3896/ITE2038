#ifndef __PAGE_H__
#define __PAGE_H__

#include "types.h"
#include <vector>
#include <cstring>

// Universal Page Layout
class Page
{
private:
	union
	{
		byte page[4096];
		/* Header Page Layout */
		struct
		{
			offset_t free_page_offset;
			offset_t root_page_offset;
			pagenum_t num_of_pages;
			table_colnum_t num_of_columns;
			byte reserved0[4064];
		};
		/* Free Page Layout */
		struct
		{
			offset_t next_free_page_offset;
			byte not_used[4088];
		};
		/* Internal/Leaf Node Page Layout */
		struct
		{
			offset_t offset;    // Parent page offset
			uint32_t isLeafNode;    // Leaf(1) / Internal(0)
			uint32_t num_keys;  // The number of keys : Maximum Internal Page(249), Leaf Page(31)
			byte reserved1[104]; // Reserved Area
			union
			{
				// One More Page Offset
				offset_t one_more_page_offset; // (leftmost child) for internal page
				// Right Sibling Page Offset
				offset_t right_sibling_page_offset; // (next page offset) for leaf page
			};
			union
			{
				// Key Pairs
				key_pair pairs[DEFAULT_INTERNAL_ORDER - 1];  // for internal page
				// Data Records
				record_t records[DEFAULT_LEAF_ORDER - 1]; // for leaf page
			};
		};
	};
public:
	Page();
	Page(const Page& copy) = delete;

	// for Header Page

	// -> Getters
	constexpr offset_t getFreePageOffset() const
	{
		return free_page_offset;
	};

	constexpr offset_t getRootPageOffset() const
	{
		return root_page_offset;
	};

	constexpr pagenum_t getNumOfPages() const
	{
		return num_of_pages;
	}

	constexpr pagenum_t getNumOfColumns() const
	{
		return num_of_columns;
	}

	// -> Setters
	void setFreePageOffset(offset_t free_page_offset);
	void setRootPageOffset(offset_t root_page_offset);
	void setNumOfPages(pagenum_t num_of_page);
	void setNumOfColumns(table_colnum_t num_of_column);

	// for Free Page

	// -> Getters
	constexpr offset_t getNextFreePageOffset() const
	{
		return this->next_free_page_offset;
	};

	// -> Setters
	void setNextFreePageOffset(offset_t next_free_page_offset);

	// for Node Page

	// -> Getters
	constexpr offset_t getParentOffset() const
	{
		return offset;
	};
	constexpr bool isLeaf() const
	{
		return isLeafNode;
	};
	constexpr int getNumOfKeys() const
	{
		return num_keys;
	};
	constexpr record_key_t getKey(key_idx_t index) const
	{
		return !isLeaf() ?
			// Internal Page : 0 ¡Â index < DEFAULT_INTERNAL_ORDER - 1
			index >= DEFAULT_INTERNAL_ORDER - 1 ? INVALID_INDEX : this->pairs[index].key :
			// Leaf Page : 0 ¡Â index < DEFAULT_LEAF_ORDER - 1
			index >= DEFAULT_LEAF_ORDER - 1 ? INVALID_INDEX : this->records[index].key;
	};
	constexpr offset_t getOffset(key_idx_t index) const
	{
		return !isLeaf() ?
			// Internal Page : 0 ¡Â index < DEFAULT_INTERNAL_ORDER
			index >= DEFAULT_INTERNAL_ORDER | index < 0 ? INVALID_INDEX :
			index == 0 ? this->one_more_page_offset : this->pairs[index - 1].offset :
			// Leaf Page : index = DEFAULT_LEAF_ORDER - 1
			index == DEFAULT_LEAF_ORDER - 1 ? this->right_sibling_page_offset : INVALID_INDEX;
	};
	constexpr int getValues(key_idx_t index, record_val_t *dest, int num_column = 0) const
	{
		// Leaf Page : 0 ¡Â index < DEFAULT_LEAF_ORDER - 1
		return isLeaf() ?
			index >= DEFAULT_LEAF_ORDER - 1 ? INVALID_INDEX
			: (std::memcpy(dest, this->records[index].values, num_column * sizeof(record_val_t)), SUCCESS) :
			INVALID_PAGE;
	}

	std::vector<record_val_t> getRecord(key_idx_t index, int num_column = 0);

	// -> Setters
	int setParentOffset(offset_t offset);
	int setLeaf();
	int setInternal();
	int setNumOfKeys(key_idx_t num_keys);
	int setKey(key_idx_t index, record_key_t key);
	int setOffset(key_idx_t index, offset_t offset);
	int setValues(key_idx_t index, const record_val_t* src, int num_column = 0);

	// -> Key Search Utility Functions
	int binarySearch(record_key_t key) const;
	int binaryRangeSearch(record_key_t key) const;

	void clear();

	void* operator&();
	const void* operator&() const;
};

#endif