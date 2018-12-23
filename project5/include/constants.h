// CONSTANTS
#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

// For debugging and testing the application
#define TESTMODE 0

// ORDER of disk-based B+ trees
constexpr auto DEFAULT_INTERNAL_ORDER = 249;
constexpr auto DEFAULT_LEAF_ORDER = 32;

// Maximums
constexpr auto MAX_NUM_COLUMNS = 16;

// Page size
constexpr auto PAGESIZE = 4096;

// Default sizes
constexpr auto DEFAULT_SIZE_OF_FREE_PAGES = 10;
constexpr auto DEFAULT_SIZE_OF_TABLES = 10;
constexpr auto DEFAULT_KEY_SIZE = 8;
constexpr auto DEFAULT_VALUE_SIZE = 120;
constexpr auto DEFAULT_RECORD_SIZE = (DEFAULT_KEY_SIZE + DEFAULT_VALUE_SIZE);

/* Error Code */
constexpr auto SUCCESS = 0;
constexpr auto INVALID_OFFSET = -1;
constexpr auto INVALID_INDEX = -2;
constexpr auto INVALID_KEY = -3;
constexpr auto INVALID_PAGE = -4;
constexpr auto INVALID_FD = -5;
constexpr auto INVALID_FILENAME = -6;
constexpr auto INVALID_TID = -7;
constexpr auto FULL_FD = -8;
constexpr auto READ_ERROR = -9;
constexpr auto WRITE_ERROR = -10;
constexpr auto SEEK_ERROR = -11;

/* Exception Code */
constexpr auto KEY_EXIST = -100;
constexpr auto KEY_NOT_FOUND = -101;

/* Offset */
constexpr auto HEADER_PAGE_NUM = 0ull;
constexpr auto HEADER_PAGE_OFFSET = 0ull;

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#define OFFSET(pagenum) ((pagenum)* PAGESIZE)
#define PGNUM(pageoffset) ((pageoffset) / PAGESIZE)

#endif