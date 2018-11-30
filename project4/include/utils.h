#ifndef __UTILS_H__
#define __UTILS_H__

#include "types.h"
#include <vector>

// Utility

/*
 * Print usage about the commands used to control the disk-based B+tree.
 */
void usage(void) noexcept;

/* 
 * Finds the appropriate place to
 * split a node that is too big into two.
 */
constexpr int cut(int length)
{
	return length % 2 == 0 ? length / 2 : length / 2 + 1;
};

void print_leaves(int table_id);
void print_tree(int table_id);
int find_range(int table_id, record_key_t key_start, record_key_t key_end, std::vector<record_t> &records);
void find_and_print_range(int table_id, record_key_t key_start, record_key_t key_end);

#endif