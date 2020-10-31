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
constexpr auto DEFAULT_SIZE_OF_TABLES     = 10;
constexpr auto DEFAULT_KEY_SIZE           = 8;
constexpr auto DEFAULT_VALUE_SIZE         = 120;
constexpr auto DEFAULT_RECORD_SIZE        = (DEFAULT_KEY_SIZE + DEFAULT_VALUE_SIZE);

/* Error Code */
enum Result {
    SUCCESS,
    INVALID_OFFSET,
    INVALID_INDEX,
    INVALID_KEY,
    INVALID_PAGE,
    INVALID_FD,
    INVALID_FILENAME,
    INVALID_TID,
    FULL_FD,
    READ_ERROR,
    WRITE_ERROR ,
    SEEK_ERROR,
    /* Exception Code */
    KEY_EXIST,
    KEY_NOT_FOUND,
};

/* Offset */
constexpr auto HEADER_PAGE_NUM = 0ull;
constexpr auto HEADER_PAGE_OFFSET = 0ull;

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#endif