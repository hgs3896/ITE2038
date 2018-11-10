#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

#include "types.h"

int init_buf(int buf_num);
void buffered_read_page(int table_id, pagenum_t pagenum, page_t* dest);
void finish_read_page(int table_id, pagenum_t pagenum);
void buffered_write_page(int table_id, pagenum_t pagenum, const page_t* src);
void flush_buf(int table_id);

#endif