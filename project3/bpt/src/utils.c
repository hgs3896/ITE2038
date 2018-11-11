#include "utils.h"
#include "file_manager.h"
#include "page_access_manager.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * The queue is used to print the tree in level order,
 * starting from the root printing each entire rank on a separate line,
 * finishing with the leaves.
 */
node* queue;

void usage(void) {
    printf( "Enter any of the following commands after the prompt > :\n"
            "\ti <k>  -- Insert <k> (an integer) as both key and value).\n"
            "\tf <k>  -- Find the value under key <k>.\n"
            "\tp <k> -- Print the path from the root to key k and its associated value.\n"
            "\tr <k1> <k2> -- Print the keys and values found in the range [<k1>, <k2>]\n"
            "\td <k>  -- Delete key <k> and its associated value.\n"
            "\tx -- Destroy the whole tree.  Start again with an empty tree of the same order.\n"
            "\tt -- Print the B+ tree.\n"
            "\tl -- Print the keys of the leaves (bottom row of the tree).\n"
            "\tv -- Toggle output of pointer addresses (\"verbose\") in tree and leaves.\n"
            "\tq -- Quit. (Or use Ctl-D.)\n"
            "\t? -- Print this help message.\n");
}

/* Finds the appropriate place to
 * split a node that is too big into two.
 */
int cut( int length ) {
    if (length % 2 == 0)
        return length / 2;
    else
        return length / 2 + 1;
}

void enqueue( offset_t new_node_offset, int depth ) {
    node *c, *new_node = malloc(sizeof(node));

    if (!new_node)
        return;

    new_node->offset = new_node_offset;
    new_node->depth = depth;
    new_node->next = NULL;

    if (queue == NULL) {
        queue = new_node;
    } else {
        c = queue;
        while (c->next != NULL) {
            c = c->next;
        }
        c->next = new_node;
    }
}

offset_t dequeue( int *depth ) {
    if (!queue)
        return HEADER_PAGE_OFFSET;

    offset_t ret;
    node * n;

    n = queue;
    queue = queue->next;
    ret = n->offset;
    if (depth)
        *depth = n->depth;
    n->next = NULL;

    free(n);
    return ret;
}

void print_leaves( int table_id, offset_t root ) {
    int i, num_keys;
    offset_t c = root;
    page_t buf;
    if (root == HEADER_PAGE_OFFSET) {
        printf("Empty tree.\n");
        return;
    }
    file_read_page(table_id, PGNUM(c), &buf);
    while (!isLeaf(&buf)) {
        c = getOffset(&buf, 0);
        file_read_page(table_id, PGNUM(c), &buf);
    }
    while (true) {
        for (i = 0, num_keys = getNumOfKeys(&buf); i < num_keys; i++) {
            printf("%lu ", getKey(&buf, i));
        }
        if (getOffset(&buf, DEFAULT_LEAF_ORDER - 1) != HEADER_PAGE_OFFSET) {
            printf(" | ");
            c = getOffset(&buf, DEFAULT_LEAF_ORDER - 1);
            file_read_page(table_id, PGNUM(c), &buf);
        }
        else break;
    }
    printf("\n");
}

void print_tree ( int table_id, offset_t root ) {
    int i = 0, search_level = 0, node_level;
    int num_keys;

    page_t buf;
    offset_t c;

    if (root == HEADER_PAGE_OFFSET) {
        printf("Empty tree.\n");
        return;
    }

    queue = NULL;
    enqueue(root, search_level);
    while ( queue != NULL ) {
        c = dequeue(&node_level);
        file_read_page(table_id, PGNUM(c), &buf);

        if (search_level < node_level) {
            puts("");
            search_level = node_level;
        }

        num_keys = getNumOfKeys(&buf);
        for (i = 0; i < num_keys; i++) {
            printf("%lu ", getKey(&buf, i));
        }
        if (!isLeaf(&buf))
            for (i = 0; i <= num_keys; i++)
                enqueue(getOffset(&buf, i), node_level + 1);
        printf("| ");
    }
    printf("\n");
}

int find_range( int table_id, offset_t root, keynum_t key_start, keynum_t key_end, record_t records[] ) {
    int i, num_found, num_keys;
    page_t page;
    offset_t p = find_leaf( table_id, root, key_start );
    if (p == HEADER_PAGE_OFFSET)
        return 0;


    file_read_page(table_id, PGNUM(p), &page);

    i = binaryRangeSearch(&page, key_start);
    num_keys = getNumOfKeys(&page);

    if (i == num_keys)
        return 0;

    num_found = 0;
    while (true) {
        for ( ; i < num_keys && getKey(&page, i) <= key_end; i++) {
            records[num_found].key = getKey(&page, i);
            getValue(&page, i, records[num_found].value);
            num_found++;
        }
        p = getOffset(&page, DEFAULT_LEAF_ORDER - 1);
        i = 0;

        if (p == HEADER_PAGE_OFFSET)
            break;

        file_read_page(table_id, PGNUM(p), &page);
        num_keys = getNumOfKeys(&page);
    }
    return num_found;
}

void find_and_print_range( int table_id, offset_t root, keynum_t key_start, keynum_t key_end) {
    int i;
    int array_size = key_end - key_start + 1;
    record_t records[array_size];
    int num_found = find_range( table_id, root, key_start, key_end, records );
    if (!num_found)
        printf("None found.\n");
    else {
        for (i = 0; i < num_found; i++)
            printf("Key: %lu\tValue: %s\n", records[i].key, records[i].value);
    }
}