// CONSTANTS
#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

// For debugging and testing the application
#define TESTMODE 0

// ORDER of disk-based B+ trees
#define DEFAULT_INTERNAL_ORDER 249
#define DEFAULT_LEAF_ORDER 32

// Page size
#define PAGESIZE 4096

// Default sizes
#define DEFAULT_SIZE_OF_FREE_PAGES 10
#define DEFAULT_SIZE_OF_TABLES     10
#define DEFAULT_KEY_SIZE           8
#define DEFAULT_VALUE_SIZE         120
#define DEFAULT_RECORD_SIZE       (DEFAULT_KEY_SIZE + DEFAULT_VALUE_SIZE)

/* Error Code */
#define SUCCESS            0
#define INVALID_OFFSET    -1
#define INVALID_INDEX     -2
#define INVALID_KEY       -3
#define INVALID_PAGE      -4
#define INVALID_FD        -5
#define INVALID_FILENAME  -6
#define INVALID_TID       -7
#define FULL_FD           -8
#define READ_ERROR        -9
#define WRITE_ERROR       -10
#define SEEK_ERROR        -11

/* Exception Code */
#define KEY_EXIST         -100
#define KEY_NOT_FOUND     -101


/* Offset */
#define HEADER_PAGE_NUM 0
#define HEADER_PAGE_OFFSET 0

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#define OFFSET(pagenum) ((pagenum) * PAGESIZE)
#define PGNUM(pageoffset) ((pageoffset) / PAGESIZE)

#endif