#ifndef __TYPES_H__
#define __TYPES_H__

#include "constants.h"
#include <cassert>
#include <iostream>
#include <list>
#include <pthread.h>

// TYPES.

// Type Redefinition
using byte = char;
using key_idx_t = int32_t ;
using record_key_t = int64_t;
using record_val_t = int64_t;
using table_colnum_t = uint64_t;
using pagenum_t = uint64_t;
using offset_t = uint64_t;
using thread = pthread_t;
using latch = pthread_mutex_t;

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
struct lock_t;
struct trx_t;

std::ostream& operator<<(std::ostream& os, const record_t& record);

enum lock_mode
{
	SHARED, EXCLUSIVE
};

enum trx_state
{
	IDLE, RUNNING, WAITING
};

struct lock_t
{
	int tid; // table id
	int rid; // record id or key
	lock_mode mode; // lock mode
	trx_t* trx; // backpointer to lock holder
};

struct trx_t
{
	int tid; // table id
	trx_state state; // transaction state
	std::list<lock_t*> trx_locks; // list of holding locks
	lock_t* wait_lock; // lock object that trx is waiting
};

#endif