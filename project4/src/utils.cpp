#include "utils.h"

#include "types.h"
#include "buffer_manager.h"
#include "index_and_file_manager.h"

#include <queue>
#include <iostream>
#include <stdlib.h>
#define TAB "\t"

/*
 * The queue is used to print the tree in level order,
 * starting from the root printing each entire rank on a separate line,
 * finishing with the leaves.
 */

 /* Node format for using queue */
struct node
{
	offset_t offset;
	int depth;
	node(offset_t offset = 0, int depth = 0): offset(offset), depth(depth){}
	node(const node& copy) : offset(copy.offset), depth(copy.depth){}
	node(const node&& temp) : offset(temp.offset), depth(temp.depth){}
	node& operator=(const node& copy)
	{
		offset = copy.offset;
		depth = copy.depth;
		return *this;
	}
	node& operator=(node&& copy)
	{
		offset = copy.offset;
		depth = copy.depth;
		return *this;
	}
};

/* Queue */
std::queue<node> queue;

void usage(void) noexcept{
    std::cout
		<<	"Enter any of the following commands after the prompt > :\n"
		TAB "i <k>  -- Insert <k> (an integer) as both key and value).\n"
		TAB "f <k>  -- Find the value under key <k>.\n"
		TAB "p <k> -- Print the path from the root to key k and its associated value.\n"
		TAB "r <k1> <k2> -- Print the keys and values found in a closed interval [<k1>, <k2>]\n"
		TAB "d <k>  -- Delete key <k> and its associated value.\n"
		TAB "D <k1> <k2>  -- Delete the records found in a closed interval [<k1>, <k2>]\n"
		TAB "x -- Destroy the whole tree.  Start again with an empty tree of the same order.\n"
		TAB "t -- Print the B+ tree.\n"
		TAB "l -- Print the keys of the leaves (bottom row of the tree).\n"
		TAB "v -- Toggle output of pointer addresses (\"verbose\") in tree and leaves.\n"
		TAB "q -- Quit. (Or use Ctl-D.)\n"
		TAB "? -- Print this help message.\n";
}

void print_leaves( int table_id ) {
    int i, num_keys;
	node n;
	
	{
		auto buf_header = BufferManager::get_frame(table_id, HEADER_PAGE_NUM);
		n.offset = BufferManager::get_page(buf_header, false)->getRootPageOffset();
		BufferManager::put_frame(buf_header);
	}

    if (n.offset == HEADER_PAGE_OFFSET) {
		std::cout << "Empty tree.\n";
        return;
    }
    auto buf = BufferManager::get_frame(table_id, PGNUM(n.offset));
    auto page = BufferManager::get_page(buf, false);

    while (!page->isLeaf()) {
        n.offset = page->getOffset(0);

        BufferManager::put_frame(buf);
        buf = BufferManager::get_frame(table_id, PGNUM(n.offset));
        page = BufferManager::get_page(buf, false);
    }
    while (true) {
        for (i = 0, num_keys = page->getNumOfKeys(); i < num_keys; i++) {
			std::cout << page->getKey(i) << ' ';
        }
        if (page->getOffset(DEFAULT_LEAF_ORDER - 1) != HEADER_PAGE_OFFSET) {
			std::cout << " | ";
			n.offset = page->getOffset(DEFAULT_LEAF_ORDER - 1);

            BufferManager::put_frame(buf);
            buf = BufferManager::get_frame(table_id, PGNUM(n.offset));
            page = BufferManager::get_page(buf, false);
        }
        else break;
    }
    BufferManager::put_frame(buf);
	std::cout << "\n";
}

void print_tree ( int table_id ) {
    int i = 0, search_level = 0;
    int num_keys;
    offset_t root;

    BufferBlock *buf_header = BufferManager::get_frame(table_id, HEADER_PAGE_NUM);
    root = BufferManager::get_page(buf_header, false)->getRootPageOffset();
    BufferManager::put_frame(buf_header);

    if (root == HEADER_PAGE_OFFSET) {
		std::cout << "Empty tree.\n";
        return;
    }

    BufferBlock *buf;
	Page *page;
	node item(root, search_level);

	queue.push(item);
	while ( !queue.empty() )
	{
		item = queue.front();
		queue.pop();
        buf = BufferManager::get_frame(table_id, PGNUM(item.offset));
        page = BufferManager::get_page(buf, false);

        if (search_level < item.depth) {
			std::cout.put('\n');
            search_level = item.depth;
        }

        num_keys = page->getNumOfKeys();
        for (i = 0; i < num_keys; i++) {
			std::cout << page->getKey(i) << ' ';
        }

		if ( !page->isLeaf() )
		{
			for ( i = 0; i <= num_keys; i++ )
			{
				assert(item.depth <= 4);
				queue.push(node { page->getOffset(i), item.depth + 1 });
			}
		}
		std::cout << "| ";

        BufferManager::put_frame(buf);
    }
	std::cout.put('\n');
}

int find_range( int table_id, record_key_t key_start, record_key_t key_end, std::vector<record_t> &records ) {
	offset_t root_offset, p;
	int num_col;

	{
		auto buf_header = BufferManager::get_frame(table_id, HEADER_PAGE_NUM);
		auto header_page = BufferManager::get_page(buf_header, false);
		root_offset = header_page->getRootPageOffset();
		num_col = header_page->getNumOfColumns();
		BufferManager::put_frame(buf_header);
	}
	
	if ( (p = find_leaf(table_id, root_offset, key_start)) == HEADER_PAGE_OFFSET)
        return 0;

    int i, num_found, num_keys;
    auto buf = BufferManager::get_frame(table_id, PGNUM(p));
    auto page = BufferManager::get_page(buf, false);

    i = page->binaryRangeSearch(key_start);
    num_keys = page->getNumOfKeys();	

    if (i == num_keys)
        return 0;

	num_found = 0;
    while (true) {
        for (; i < num_keys && page->getKey(i) <= key_end; i++) {
            records[num_found].key = page->getKey(i);
            page->getValues(i, records[num_found].values, num_col);
            num_found++;
        }
		p = page->getOffset(DEFAULT_LEAF_ORDER - 1);
        i = 0;

        if (p == HEADER_PAGE_OFFSET)
            break;

        BufferManager::put_frame(buf);
        buf = BufferManager::get_frame(table_id, PGNUM(p));
        page = BufferManager::get_page(buf, false);
        num_keys = page->getNumOfKeys();
    }
    BufferManager::put_frame(buf);
    
    return num_found;
}

void find_and_print_range( int table_id, record_key_t key_start, record_key_t key_end) {
    int i;
	if ( key_start > key_end )
	{
		record_key_t t = key_start;
		key_start = key_end;
		key_end = t;
	}

	std::vector<record_t> results;
	results.reserve(key_end - key_start + 1);
	int num_found = find_range( table_id, key_start, key_end, results );
    if (!num_found)
		std::cout << "None found.\n";
    else {
		for ( i = 0; i < num_found; i++ )
			std::cout << results[i] << "\n";
    }
}

std::ostream& operator<<(std::ostream& os, const record_t& record)
{
	return os << "Key: " << record.key << ", " << record.values[0] << '\n';
}