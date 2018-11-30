#ifndef __TYPES_H__
#define __TYPES_H__

#include "constants.h"
#include <cassert>
#include <iostream>

// TYPES.

// Type Redefinition
using byte = char;
using key_idx_t = int32_t ;
using record_key_t = int64_t;
using record_val_t = int64_t;
using table_colnum_t = uint64_t;
using pagenum_t = uint64_t;
using offset_t = uint64_t;

// Type Definition

/* Record format for storing the records */
struct record_t
{
    record_key_t key;
	record_val_t values[MAX_NUM_COLUMNS - 1];
};

/* Key Pair */
struct key_pair
{
    record_key_t key;
    offset_t offset;
};

class Page;

std::ostream& operator<<(std::ostream& os, const record_t& record);

#endif