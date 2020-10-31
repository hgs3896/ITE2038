#ifndef __BPT_H__
#define __BPT_H__

#define Version "1.05"

#include "common.h"
#include "utils.h"
#include "wrapper_funcs.h"

/*
 * Initialize buffer pool with given number and buffer manager.
 */
int init_db(int buf_num);

/*
 * Open existing data file using ‘pathname’ or create one if not existed.
 * If success, return table_id.
 */
int open_table(char * pathname, int num_column);

/*
 * Write the pages relating to this table to disk and close the table.
 *  - Write all pages of this table from buffer to disk and discard the table id.
 *  - If success, return 0. Otherwise, return non-zero value.
 */
int close_table(int table_id);

/*
 * Destroy buffer manager.
 *  - Flush all data from buffer and destroy allocated buffer.
 *  - If success, return 0. Otherwise, return non-zero value.
 */
int shutdown_db(void);

/*
 *  Find the record containing input ‘key’.
 *  If found matching ‘key’, return matched ‘value’ string. Otherwise, return NULL.
 */
int64_t* find(int table_id, int64_t key);

/*
 *  Insert input ‘key/value’ (record) to data file at the right place.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int insert(int table_id, int64_t key, int64_t* values);

/*
 *  Find the matching record and delete it if found.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int erase(int table_id, int64_t key);

/*
 *  Join the tables specified by the query and return a sum of all keys in the query result.
 */
int64_t join(const char* query);

#endif /* __BPT_H__ */