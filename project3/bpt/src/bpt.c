#include "bpt.h"

#include "index_and_file_manager.h"
#include "buffer_manager.h"
#include "disk_manager.h"

#include "page_access_manager.h"

/*
 * Initialize buffer pool with given number and buffer manager.
 */
int init_db(int buf_num) {
	buf_init(buf_num);
}

/*
 * Open existing data file using ‘pathname’ or create one if not existed.
 * If success, return table_id.
 */
int open_table(char * pathname) {
	file_open_table(pathname);
}

/*
 * Write the pages relating to this table to disk and close the table.
 *  - Write all pages of this table from buffer to disk and discard the table id.
 *  - If success, return 0. Otherwise, return non-zero value.
 */
int close_table(int table_id) {
	file_close_table(table_id);
}

/*
 * Destroy buffer manager.
 *  - Flush all data from buffer and destroy allocated buffer.
 *  - If success, return 0. Otherwise, return non-zero value.
 */
int shutdown_db(void) {
	buf_shutdown();
}

/*
 *  Find the record containing input ‘key’.
 *  If found matching ‘key’, return matched ‘value’ string. Otherwise, return NULL.
 */
char * find(int table_id, keynum_t key) {
	record_t* record;

	buffer_frame_t *buf_header = buf_get_frame(table_id, HEADER_PAGE_NUM);
	offset_t root_offset = getRootPageOffset(buf_get_page(buf_header, false));
	buf_put_frame(buf_header);

	if ((record = find_record(table_id, root_offset, key)) == NULL)
		return NULL;

	char * ret = malloc(DEFAULT_VALUE_SIZE);
	strncpy(ret, record->value, DEFAULT_VALUE_SIZE);
	free(record);
	return ret;
}

/*
 *  Insert input ‘key/value’ (record) to data file at the right place.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int insert(int table_id, keynum_t key, char * value) {
	record_t record;

	// Set the data
	record.key = key;
	strncpy(record.value, value, 120);

	buffer_frame_t *buf_header = buf_get_frame(table_id, HEADER_PAGE_NUM);
	offset_t root_offset = getRootPageOffset(buf_get_page(buf_header, false));
	buf_put_frame(buf_header);

	// Insert the record
	root_offset = insert_record(table_id, root_offset, &record);

	if (root_offset == KEY_EXIST)
		return KEY_EXIST;

	// Update the root offset
	buf_header = buf_get_frame(table_id, HEADER_PAGE_NUM);
	setRootPageOffset(buf_get_page(buf_header, true), root_offset);
	buf_put_frame(buf_header);

	return SUCCESS;
}

/*
 *  Find the matching record and delete it if found.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int delete(int table_id, keynum_t key) {
	buffer_frame_t *buf_header = buf_get_frame(table_id, HEADER_PAGE_NUM);
	offset_t root_offset = getRootPageOffset(buf_get_page(buf_header, false));
	buf_put_frame(buf_header);

	// Insert the record
	root_offset = delete_record(table_id, root_offset, key );

	if (root_offset != KEY_EXIST) {
		// Get the buffer block
		buf_header = buf_get_frame(table_id, HEADER_PAGE_NUM);
		// Reset the Root Page Offset.
		setRootPageOffset(buf_get_page(buf_header, true), root_offset);
		// Put it back to the buffer.
		buf_put_frame(buf_header);
		return 0;
	}
	return -1;
}