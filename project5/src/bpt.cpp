#include "bpt.h"

#include "index_and_file_manager.h"
#include "buffer_manager.h"
#include "disk_manager.h"

#include <cstring>
#include <string>

/*
 * Initialize buffer pool with given number and buffer manager.
 */
int init_db(int buf_num) {
	return BufferManager::init(buf_num);
}

/*
 * Open existing data file using ‘pathname’ or create one if not existed.
 * If success, return table_id.
 */
int open_table(char * pathname, int num_column) {
	const auto tid = file_open_table(pathname, num_column);
	return tid;
}

/*
 * Write the pages relating to this table to disk and close the table.
 *  - Write all pages of this table from buffer to disk and discard the table id.
 *  - If success, return 0. Otherwise, return non-zero value.
 */
int close_table(int table_id) {
	return file_close_table(table_id);
}

/*
 * Destroy buffer manager.
 *  - Flush all data from buffer and destroy allocated buffer.
 *  - If success, return 0. Otherwise, return non-zero value.
 */
int shutdown_db(void) {
	BufferManager::shutdown();
	return SUCCESS;
}

/*
 *  Find the record containing input ‘key’.
 *  If found matching ‘key’, return matched ‘value’ string. Otherwise, return NULL.
 */
int64_t* find(int table_id, int64_t key) {
	record_t* record;
	offset_t root_offset;
	int num_cols;
	
	{
		auto buf_header = BufferManager::get_frame(table_id, HEADER_PAGE_NUM);
		auto page = BufferManager::get_page(buf_header, false);
		root_offset = page->getRootPageOffset();
		num_cols = page->getNumOfColumns();
		BufferManager::put_frame(buf_header);
	}
	
	if ((record = find_record(table_id, root_offset, key)) == nullptr || num_cols < 2 || num_cols > MAX_NUM_COLUMNS )
		return NULL;

	// Copy the contents
	int64_t* result_values = new int64_t[num_cols - 1];
	std::memcpy(result_values, record->values, (num_cols - 1) * sizeof(record_val_t));
	
	// remove record
	delete record;

	// return the values
	return result_values;
}

/*
 *  Insert input ‘key/value’ (record) to data file at the right place.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int insert(int table_id, int64_t key, int64_t* values) {
	record_t record = record_t();
	offset_t root_offset;
	int num_cols;

	{
		auto buf_header = BufferManager::get_frame(table_id, HEADER_PAGE_NUM);
		auto page = BufferManager::get_page(buf_header, false);
		root_offset = page->getRootPageOffset();
		num_cols = page->getNumOfColumns();
		BufferManager::put_frame(buf_header);
	}

	// Set the data
	record.key = key;
	std::memcpy(record.values, values, (num_cols - 1) * sizeof(record_val_t));

	// Insert the record
	root_offset = insert_record(table_id, root_offset, &record);

	if (root_offset == KEY_EXIST)
		return KEY_EXIST;
	
	{
		// Update the root offset
		auto buf_header = BufferManager::get_frame(table_id, HEADER_PAGE_NUM);
		BufferManager::get_page(buf_header, true)->setRootPageOffset(root_offset);
		BufferManager::put_frame(buf_header);
	}

	return SUCCESS;
}

/*
 *  Find the matching record and delete it if found.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int erase(int table_id, int64_t key) {
	auto buf_header = BufferManager::get_frame(table_id, HEADER_PAGE_NUM);
	offset_t root_offset = BufferManager::get_page(buf_header, false)->getRootPageOffset();
	BufferManager::put_frame(buf_header);

	// Insert the record
	root_offset = delete_record(table_id, root_offset, key );

	if (root_offset != KEY_EXIST) {
		// Get the buffer block
		buf_header = BufferManager::get_frame(table_id, HEADER_PAGE_NUM);
		// Reset the Root Page Offset.
		BufferManager::get_page(buf_header, true)->setRootPageOffset(root_offset);
		// Put it back to the buffer.
		BufferManager::put_frame(buf_header);
		return 0;
	}
	return -1;
}