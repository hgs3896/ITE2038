#ifndef __BPT_H__
#define __BPT_H__

#define Version "1.01"

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Initialize buffer pool with given number and buffer manager.
 */
int init_db(int buf_num);

/*
 * Open existing data file using ‘pathname’ or create one if not existed.
 * If success, return table_id.
 */
int open_table(char * pathname);

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
char * find(int table_id, keynum_t key);

/*
 *  Insert input ‘key/value’ (record) to data file at the right place.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int insert(int table_id, keynum_t key, char * value);

/*
 *  Find the matching record and delete it if found.
 *  If success, return 0. Otherwise, return non-zero value.
 */
int delete(int table_id, keynum_t key);

#endif /* __BPT_H__ */