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

// Default size of free pages
#define DEFAULT_SIZE_OF_FREE_PAGES 10
#define DEFAULT_SIZE_OF_TABLES     10

/* Error Code */
#define SUCCESS            0
#define INVALID_OFFSET    -1
#define INVALID_INDEX     -2
#define INVALID_KEY       -3
#define INVALID_PAGE      -4
#define FULL_FD           -5
#define INVALID_FILENAME  -6
#define READ_ERROR        -7
#define WRITE_ERROR       -8
#define SEEK_ERROR        -9

/* Exception Code */
#define KEY_EXIST         -100
#define KEY_NOT_FOUND     -101


/* Offset */
#define HEADER_PAGE_NUM 0
#define HEADER_PAGE_OFFSET 0

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#endif