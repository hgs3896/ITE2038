/*
 *  bpt.c  
 */

/*
 *
 *  bpt:  B+ Tree Implementation
 *  Copyright (C) 2010-2016  Amittai Aviram  http://www.amittai.com
 *  All rights reserved.
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, 
 *  this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *  this list of conditions and the following disclaimer in the documentation 
 *  and/or other materials provided with the distribution.
 
 *  3. Neither the name of the copyright holder nor the names of its 
 *  contributors may be used to endorse or promote products derived from this 
 *  software without specific prior written permission.
 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 *  POSSIBILITY OF SUCH DAMAGE.
 
 *  Author:  Amittai Aviram 
 *    http://www.amittai.com
 *    amittai.aviram@gmail.edu or afa13@columbia.edu
 *  Original Date:  26 June 2010
 *  Last modified: 17 June 2016
 *
 *  This implementation demonstrates the B+ tree data structure
 *  for educational purposes, includin insertion, deletion, search, and display
 *  of the search path, the leaves, or the whole tree.
 *  
 *  Must be compiled with a C99-compliant C compiler such as the latest GCC.
 *
 *  Usage:  bpt [order]
 *  where order is an optional argument
 *  (integer MIN_ORDER <= order <= MAX_ORDER)
 *  defined as the maximal number of pointers in any node.
 *
 */

#include "bpt.h"

// GLOBALS.

/* The queue is used to print the tree in
 * level order, starting from the root
 * printing each entire rank on a separate
 * line, finishing with the leaves.
 */
node* queue = NULL;

/* The user can toggle on and off the ""
 * property, which causes the pointer addresses
 * to be printed out in hexadecimal notation
 * next to their corresponding keys.
 */
bool _output = false;

/* File descriptor for reading and writing database file. */
int fd;

/* Header Page */
page_t header;

/* Page Number */
const pagenum_t PAGE_NOT_FOUND = -1l;
const pagenum_t FILEDESCRIPTOR_ERROR = -2;

/* Prints the B+ tree in the command
 * line in level (rank) order, with the 
 * keys in each node and the '|' symbol
 * to separate nodes.
 * With the _output flag set.
 * the values of the pointers corresponding
 * to the keys also appear next to their respective
 * keys, in hexadecimal notation.
 */
void print_tree( pagenum_t pagenum ) {
    if(!SEEK(OFFSET(pagenum))){
        perror("file seek error");
        return;
    }
    page_t buf;
    if(!READ(buf)){
        perror("file read error");
        return;
    }
    offset_t n;
    int i, j;
    if(isLeaf(&buf) == false){
        // Internal Page
        queue = NULL;
        enqueue(OFFSET(pagenum));
        while( queue != NULL ) {
            n = dequeue();
            SEEK(OFFSET(n));
            READ(buf);

            for (i = 0, j = getNumberOfKeys(&buf); i < j; i++) {
                printf("%d ", getKey(&buf, i));
            }
            if (!buf.isLeaf){
                for (i = 0, j = getNumberOfKeys(&buf); i <= j; i++)
                    enqueue(getOffset(&buf, i));
            }
        }
        printf("| ");
    }else{
        // Leaf Page
    }

    printf("\n");
}


/* Finds the record_t under a given key and prints an
 * appropriate message to stdout.
 */
void find_and_print(node * root, uint64_t key ) {
    record_t * r = find_record(root, key);
    if (r == NULL)
        printf("record_t not found under key %ul.\n", key);
    else 
        printf("record_t at %lx -- key %ul, value %s.\n",
                (unsigned long)r, key, r->value);
    free(r);
}


/* Finds and prints the keys, pointers, and values within a range
 * of keys between key_start and key_end, including both bounds.
 */
/*
void find_and_print_range( node * root, uint64_t key_start, uint64_t key_end ) {
    int i;
    int array_size = key_end - key_start + 1;
    int returned_keys[array_size];
    void * returned_pointers[array_size];
    int num_found = find_range( root, key_start, key_end,
            returned_keys, returned_pointers );
    if (!num_found)
        printf("None found.\n");
    else {
        for (i = 0; i < num_found; i++)
            printf("Key: %d   Location: %lx  Value: %d\n",
                    returned_keys[i],
                    (unsigned long)returned_pointers[i],
                    ((record_t *)
                     returned_pointers[i])->value);
    }
}
*/


/* Finds keys and their pointers, if present, in the range specified
 * by key_start and key_end, inclusive.  Places these in the arrays
 * returned_keys and returned_pointers, and returns the number of
 * entries found.
 */
/*
int find_range( offset_t root, uint64_t key_start, uint64_t key_end, int returned_keys[], void * returned_pointers[]) {
    int i, num_found;
    num_found = 0;
    offset_t n = find_leaf( root, key_start );
    if (n == NULL) return 0;
    for (i = 0; i < n->num_keys && n->keys[i] < key_start; i++) ;
    if (i == n->num_keys) return 0;
    while (n != NULL) {
        for ( ; i < n->num_keys && n->keys[i] <= key_end; i++) {
            returned_keys[num_found] = n->keys[i];
            returned_pointers[num_found] = n->pointers[i];
            num_found++;
        }
        n = n->pointers[order - 1];
        i = 0;
    }
    return num_found;
}
*/


/* Traces the path from the root to a leaf, searching
 * by key.  Displays information about the path
 * if the  flag is set.
 * Returns the leaf containing the given key.
 */
offset_t find_leaf(offset_t root, uint64_t key)
{
    if ( fd <= 0 )
        return 0; // error

    int i = 0, j;
    offset_t c = root;
    if ( c == 0 )
    {
        return c;
    }

    page_t buf;
    while ( c != 0 && SEEK(c) >= 0 && READ(buf) >= 0 && !isLeaf(&buf) )
    {
        i = 0, j = getNumberOfKeys(&buf);
        while ( i < j )
        {
            if ( key >= getKey(&buf.i) ) i++;
            else break;
        }
        c = getOffset(&buf,i);
    }
    return c;
}


/* Finds and returns the record_t to which
 * a key refers.
 */
record_t * find_record( offset_t root, uint64_t key )
{
    int i = 0, j;
    offset_t c = find_leaf(root, key);
    if ( c == 0 ) return NULL;

    page_t buf;
    SEEK(c);
    READ(buf);
    return getRecord(&buf, key);
}

/*
    Find the record_t containing input ‘key’.
    If found matching ‘key’, return matched ‘value’ string. Otherwise, return NULL.
*/

char * find (int64_t key){
    return find_record(header.root_page_offset, key);
    // 이미 동일한 키가 있는지 검사한다.
    // 찾으면 value를 return한다.
    // 못 찾았으면 NULL을 return한다.
}

/* Finds the appropriate place to
 * split a node that is too big into two.
 */
int cut( int length ) {
    if (length % 2 == 0)
        return length/2;
    else
        return length/2 + 1;
}


// INSERTION

/* Creates a new general node, which can be adapted
 * to serve as either a leaf or an internal node.
 */
offset_t make_internal(void)
{
    pagenum_t new_page = file_alloc_page();
    if ( new_page < 0 )
        return PAGE_NOT_FOUND;
    page_t buf;
    setNumberOfKeys(&buf, 0);
    setLeaf(false);
    SEEK(OFFSET(new_page));
    WRITE(buf);
    return OFFSET(new_page);
}

/* Creates a new leaf by creating a node
 * and then adapting it appropriately.
 */
offset_t make_leaf(void)
{
    pagenum_t new_page = file_alloc_page();
    if ( new_page < 0 )
        return PAGE_NOT_FOUND;
    page_t buf;
    setNumberOfKeys(&buf, 0);
    setLeaf(true);
    SEEK(OFFSET(new_page));
    WRITE(buf);
    return OFFSET(new_page);
}


/* Find the index of the parent's pointer to the node to the left of the key to be inserted.
 */
int get_left_index(offset_t parent, offset_t left) {
    page_t p;
    SEEK(parent);
    READ(p);
    int left_index = 0, num_keys = getNumberOfKeys(&p);

    while (left_index <= num_keys && getOffset(&p, left_index) != left)
        left_index++;
    return left_index;
}

/* Inserts a new pointer to a record_t and its corresponding
 * key into a leaf.
 * Returns the altered leaf.
 */
offset_t insert_into_leaf(offset_t leaf, uint64_t key, record_t * pointer)
{
    page_t buf;
    SEEK(leaf);
    READ(buf);
    int i, insertion_point, num_keys;

    insertion_point = 0;
    num_keys = getNumberOfKeys(&buf);
    while ( insertion_point < num_keys && getKey(&buf, insertion_point) < key )
        insertion_point++;

    for ( i = num_keys; i > insertion_point; i-- )
    {
        moveRecord(&buf, i - 1, i);
    }
    setRecord(&buf, insertion_point, pointer);
    setNumberOfKeys(buf.num_keys++);
    WRITE(buf);
    return leaf;
}

/* Inserts a new key and pointer
 * to a new record_t into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.
 */
offset_t insert_into_leaf_after_splitting(offset_t root, offset_t leaf, uint64_t key, record_t * pointer)
{
    page_t left_page, right_page;
    offset_t new_leaf;
    int insertion_index, split, i, j;
    key_t new_key;

    SEEK(leaf);
    READ(left_page);
    CLEAR(right_page);
    new_leaf = make_leaf();

    // 삽입 위치를 찾고
    insertion_index = 0;
    while ( insertion_index < DEFAULT_LEAF_ORDER - 1 && left_page.data[insertion_index].key < key )
        insertion_index++;

    // 나누고
    split = cut(DEFAULT_LEAF_ORDER - 1);

    // 삽입할 위치가 분할된 위치보다 크면, 오른쪽에 삽입하고 작으면 왼쪽에 삽입한다.
    if ( insertion_index < split )
    {
        for ( i = split - 1, j = l.num_keys; i < j; ++i )
        {
            right_page.data[right_page.num_keys++] = left_page.data[i];
        }
        for ( i = split - 1; i > insertion_index; --i )
        {
            left_page.data[i] = left_page.data[i - 1];
        }
        left_page.num_keys = split;
        left_page.data[i] = *pointer;
        left_page.offset = root;
    }
    else
    {
        for ( i = split; i < insertion_index; ++i )
        {
            right_page.data[right_page.num_keys++] = left_page.data[i];
        }
        right_page.data[right_page.num_keys++] = *pointer;
        for ( i = insertion_index; i < left_page.num_keys; ++i )
        {
            right_page.data[right_page.num_keys++] = left_page.data[i];
        }
        left_page.num_keys = split;
        left_page.offset = root;
    }

    new_key = right_page.data[0].key;

    SEEK(leaf);
    WRITE(left_page);
    SEEK(new_leaf);
    WRITE(right_page);

    return insert_into_parent(root, leaf, new_key, new_leaf);
}

/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
offset_t insert_into_parent(offset_t root, offset_t left, uint64_t key, offset_t right)
{

    int left_index;
    offset_t parent;
    page_t buf;
    SEEK(left);
    READ(buf);

    parent = buf.offset;

    /* Case: new root. */

    if ( parent == 0 )
        return insert_into_new_root(left, key, right);

    /* Case: leaf or node. (Remainder of function body.)
     */

     /* Find the parent's pointer to the left node.
      */

    left_index = get_left_index(parent, left);

    /* Simple case: the new key fits into the node.
     */

    if ( buf.num_keys < DEFAULT_INTERNAL_ORDER - 1 )
        return insert_into_node(root, parent, left_index, key, right);

    /* Harder case:  split a node in order
     * to preserve the B+ tree properties.
     */

    return insert_into_node_after_splitting(root, parent, left_index, key, right);
}

/* Inserts a new key and pointer to a node into a node into which these can fit without violating the B+ tree properties.
 */
offset_t insert_into_node(offset_t root, offset_t n, int left_index, uint64_t key, offset_t right) {
    int i;
    page_t p;
    SEEK(n);
    READ(p);
    for( i = p.num_keys; i > left_index; i-- ){
        p.pair[i+1].offset = p.pair[i].offset;
        p.pair[i].key = p.pair[i-1].key;
    }
    p.pair[left_index + 1].offset = right;
    p.pair[left_index].key = key;
    p.num_keys++;
    WRITE(p);
    return root;
}


/* Inserts a new key and pointer to a node
 * into a node, causing the node's size to exceed
 * the order, and causing the node to split into two.
 */
offset_t insert_into_node_after_splitting(offset_t root, offset_t old_node, int left_index, uint64_t key, offset_t right) {

    int i, j, split, k_prime;
    offset_t new_node, child;
    page_t old_page, new_page;
    
    /* First create a temporary set of keys and pointers
     * to hold everything in order, including
     * the new key and pointer, inserted in their
     * correct places. 
     * Then create a new node and copy half of the 
     * keys and pointers to the old node and
     * the other half to the new.
     */

    SEEK(old_node);
    READ(old_page);

    /*
    for (i = 0, j = 0; i < old_page.num_keys + 1; i++, j++) {
        if (j == left_index + 1) j++;
        temp_pointers[j] = old_node->pointers[i];
    }

    for (i = 0, j = 0; i < old_node->num_keys; i++, j++) {
        if (j == left_index) j++;
        temp_keys[j] = old_node->keys[i];
    }

    temp_pointers[left_index + 1] = right;
    temp_keys[left_index] = key;
    */

    /* Create the new node and copy half the keys and pointers to the old and half to the new. */  
    split = cut(DEFAULT_INTERNAL_ORDER);

    for(i=0;i<old_page.num_keys && old_page.pair[i].key != left_index;++i);
    for(j=i;)old_page.num_keys

    new_node = make_node();
    old_page.num_keys = 0;

    SEEK(new_node);
    READ(new_page);
    
    for (i = )

    for (i = 0; i < split - 1; i++) {
        old_node->pointers[i] = temp_pointers[i];
        old_node->keys[i] = temp_keys[i];
        old_node->num_keys++;
    }
    old_node->pointers[i] = temp_pointers[i];
    k_prime = temp_keys[split - 1];
    for (++i, j = 0; i < DEFAULT_INTERNAL_ORDER; i++, j++) {
        new_node->pointers[j] = temp_pointers[i];
        new_node->keys[j] = temp_keys[i];
        new_node->num_keys++;
    }
    new_node->pointers[j] = temp_pointers[i];
    free(temp_pointers);
    free(temp_keys);
    new_node->parent = old_node->parent;
    for (i = 0; i <= new_node->num_keys; i++) {
        child = new_node->pointers[i];
        child->parent = new_node;
    }

    /* Insert a new key into the parent of the two
     * nodes resulting from the split, with
     * the old node to the left and the new to the right.
     */

    return insert_into_parent(root, old_node, k_prime, new_node);
}


/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
offset_t insert_into_new_root(offset_t left, uint64_t key, offset_t right) {
    page_t page;
    offset_t root = make_node();
    
    SEEK(root);    
    READ(page);
    page.one_more_page_offset = left;
    page.pair[0].key = key;
    page.pair[0].offset = right;
    page.num_keys++;
    WRITE(page);

    SEEK(left);    
    READ(page);
    page.offset = root;
    WRITE(page);

    SEEK(right);    
    READ(page);
    page.offset = root;    
    WRITE(page);

    return root;
}



/* First insertion: start a new tree.
 */
offset_t start_new_tree(uint64_t key, record_t * pointer) {
    offset_t root = make_leaf();
    page_t page;
    SEEK(root);
    READ(page);
    page.pair[page.num_keys++] = *pointer;
    page.offset = 0;
    WRITE(page);
    return root;
}

int insert (int64_t key, char * value){
    insert_record(header.root_page_offset, key, value);
}

/* Master insertion function.
 * Inserts a key and an associated value into
 * the B+ tree, causing the tree to be adjusted
 * however necessary to maintain the B+ tree
 * properties.
 */
offset_t insert_record( offset_t root, uint64_t key, const char * value )
{

    record_t * pointer;
    node * leaf;

    /* The current implementation ignores
     * duplicates.
     */

     /* Create a new record_t for the
      * value.
      */
    pointer = make_record_t(value);


    /* Case: the tree does not exist yet.
     * Start a new tree.
     */

    if ( root == NULL )
        return start_new_tree(key, pointer);


    /* Case: the tree already exists.
     * (Rest of function body.)
     */

    leaf = find_leaf(root, key, false);

    /* Case: leaf has room for key and pointer.
     */

    if ( leaf->num_keys < order - 1 )
    {
        leaf = insert_into_leaf(leaf, key, pointer);
        return root;
    }


    /* Case:  leaf must be split.
     */

    return insert_into_leaf_after_splitting(root, leaf, key, pointer);
}

/*
    Insert input ‘key/value’ (record_t) to data file at the right place.
    If success, return 0. Otherwise, return non-zero value.
*/

int insert (int64_t key, char * value){
    if (find_record(root, key) != 0)
        return root;
    // 이미 동일한 키가 있는지 검사하고, 삽입각을 잡는다.
    // 삽입각에 삽입한다.
    // 정상적으로 삽입했다면 0, 아니면 0이 아닌 값이다.
}

#if 0

// DELETION.

/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node
 * is the leftmost child), returns -1 to signify
 * this special case.
 */
int get_neighbor_index( node * n ) {

    int i;

    /* Return the index of the key to the left
     * of the pointer in the parent pointing
     * to n.  
     * If n is the leftmost child, this means
     * return -1.
     */
    for (i = 0; i <= n->parent->num_keys; i++)
        if (n->parent->pointers[i] == n)
            return i - 1;

    // Error state.
    printf("Search for nonexistent pointer to node in parent.\n");
    printf("Node:  %#lx\n", (unsigned long)n);
    exit(EXIT_FAILURE);
}


node * remove_entry_from_node(node * n, uint64_t key, node * pointer) {

    int i, num_pointers;

    // Remove the key and shift other keys accordingly.
    i = 0;
    while (n->keys[i] != key)
        i++;
    for (++i; i < n->num_keys; i++)
        n->keys[i - 1] = n->keys[i];

    // Remove the pointer and shift other pointers accordingly.
    // First determine number of pointers.
    num_pointers = n->is_leaf ? n->num_keys : n->num_keys + 1;
    i = 0;
    while (n->pointers[i] != pointer)
        i++;
    for (++i; i < num_pointers; i++)
        n->pointers[i - 1] = n->pointers[i];


    // One key fewer.
    n->num_keys--;

    // Set the other pointers to NULL for tidiness.
    // A leaf uses the last pointer to point to the next leaf.
    if (n->is_leaf)
        for (i = n->num_keys; i < order - 1; i++)
            n->pointers[i] = NULL;
    else
        for (i = n->num_keys + 1; i < order; i++)
            n->pointers[i] = NULL;

    return n;
}


node * adjust_root(node * root) {

    node * new_root;

    /* Case: nonempty root.
     * Key and pointer have already been deleted,
     * so nothing to be done.
     */

    if (root->num_keys > 0)
        return root;

    /* Case: empty root. 
     */

    // If it has a child, promote 
    // the first (only) child
    // as the new root.

    if (!root->is_leaf) {
        new_root = root->pointers[0];
        new_root->parent = NULL;
    }

    // If it is a leaf (has no children),
    // then the whole tree is empty.

    else
        new_root = NULL;

    free(root->keys);
    free(root->pointers);
    free(root);

    return new_root;
}


/* Coalesces a node that has become
 * too small after deletion
 * with a neighboring node that
 * can accept the additional entries
 * without exceeding the maximum.
 */
node * coalesce_nodes(node * root, node * n, node * neighbor, int neighbor_index, int k_prime) {

    int i, j, neighbor_insertion_index, n_end;
    node * tmp;

    /* Swap neighbor with node if node is on the
     * extreme left and neighbor is to its right.
     */

    if (neighbor_index == -1) {
        tmp = n;
        n = neighbor;
        neighbor = tmp;
    }

    /* Starting point in the neighbor for copying
     * keys and pointers from n.
     * Recall that n and neighbor have swapped places
     * in the special case of n being a leftmost child.
     */

    neighbor_insertion_index = neighbor->num_keys;

    /* Case:  nonleaf node.
     * Append k_prime and the following pointer.
     * Append all pointers and keys from the neighbor.
     */

    if (!n->is_leaf) {

        /* Append k_prime.
         */

        neighbor->keys[neighbor_insertion_index] = k_prime;
        neighbor->num_keys++;


        n_end = n->num_keys;

        for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
            neighbor->keys[i] = n->keys[j];
            neighbor->pointers[i] = n->pointers[j];
            neighbor->num_keys++;
            n->num_keys--;
        }

        /* The number of pointers is always
         * one more than the number of keys.
         */

        neighbor->pointers[i] = n->pointers[j];

        /* All children must now point up to the same parent.
         */

        for (i = 0; i < neighbor->num_keys + 1; i++) {
            tmp = (node *)neighbor->pointers[i];
            tmp->parent = neighbor;
        }
    }

    /* In a leaf, append the keys and pointers of
     * n to the neighbor.
     * Set the neighbor's last pointer to point to
     * what had been n's right neighbor.
     */

    else {
        for (i = neighbor_insertion_index, j = 0; j < n->num_keys; i++, j++) {
            neighbor->keys[i] = n->keys[j];
            neighbor->pointers[i] = n->pointers[j];
            neighbor->num_keys++;
        }
        neighbor->pointers[order - 1] = n->pointers[order - 1];
    }

    root = delete_entry(root, n->parent, k_prime, n);
    free(n->keys);
    free(n->pointers);
    free(n); 
    return root;
}


/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the
 * maximum
 */
node * redistribute_nodes(node * root, node * n, node * neighbor, int neighbor_index, 
        int k_prime_index, int k_prime) {  

    int i;
    node * tmp;

    /* Case: n has a neighbor to the left. 
     * Pull the neighbor's last key-pointer pair over
     * from the neighbor's right end to n's left end.
     */

    if (neighbor_index != -1) {
        if (!n->is_leaf)
            n->pointers[n->num_keys + 1] = n->pointers[n->num_keys];
        for (i = n->num_keys; i > 0; i--) {
            n->keys[i] = n->keys[i - 1];
            n->pointers[i] = n->pointers[i - 1];
        }
        if (!n->is_leaf) {
            n->pointers[0] = neighbor->pointers[neighbor->num_keys];
            tmp = (node *)n->pointers[0];
            tmp->parent = n;
            neighbor->pointers[neighbor->num_keys] = NULL;
            n->keys[0] = k_prime;
            n->parent->keys[k_prime_index] = neighbor->keys[neighbor->num_keys - 1];
        }
        else {
            n->pointers[0] = neighbor->pointers[neighbor->num_keys - 1];
            neighbor->pointers[neighbor->num_keys - 1] = NULL;
            n->keys[0] = neighbor->keys[neighbor->num_keys - 1];
            n->parent->keys[k_prime_index] = n->keys[0];
        }
    }

    /* Case: n is the leftmost child.
     * Take a key-pointer pair from the neighbor to the right.
     * Move the neighbor's leftmost key-pointer pair
     * to n's rightmost position.
     */

    else {  
        if (n->is_leaf) {
            n->keys[n->num_keys] = neighbor->keys[0];
            n->pointers[n->num_keys] = neighbor->pointers[0];
            n->parent->keys[k_prime_index] = neighbor->keys[1];
        }
        else {
            n->keys[n->num_keys] = k_prime;
            n->pointers[n->num_keys + 1] = neighbor->pointers[0];
            tmp = (node *)n->pointers[n->num_keys + 1];
            tmp->parent = n;
            n->parent->keys[k_prime_index] = neighbor->keys[0];
        }
        for (i = 0; i < neighbor->num_keys - 1; i++) {
            neighbor->keys[i] = neighbor->keys[i + 1];
            neighbor->pointers[i] = neighbor->pointers[i + 1];
        }
        if (!n->is_leaf)
            neighbor->pointers[i] = neighbor->pointers[i + 1];
    }

    /* n now has one more key and one more pointer;
     * the neighbor has one fewer of each.
     */

    n->num_keys++;
    neighbor->num_keys--;

    return root;
}


/* Deletes an entry from the B+ tree.
 * Removes the record_t and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
node * delete_entry( node * root, node * n, uint64_t key, void * pointer ) {

    int min_keys;
    node * neighbor;
    int neighbor_index;
    int k_prime_index, k_prime;
    int capacity;

    // Remove key and pointer from node.

    n = remove_entry_from_node(n, key, pointer);

    /* Case:  deletion from the root. 
     */

    if (n == root) 
        return adjust_root(root);


    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */

    /* Determine minimum allowable size of node,
     * to be preserved after deletion.
     */

    min_keys = n->is_leaf ? cut(order - 1) : cut(order) - 1;

    /* Case:  node stays at or above minimum.
     * (The simple case.)
     */

    if (n->num_keys >= min_keys)
        return root;

    /* Case:  node falls below minimum.
     * Either coalescence or redistribution
     * is needed.
     */

    /* Find the appropriate neighbor node with which
     * to coalesce.
     * Also find the key (k_prime) in the parent
     * between the pointer to node n and the pointer
     * to the neighbor.
     */

    neighbor_index = get_neighbor_index( n );
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    k_prime = n->parent->keys[k_prime_index];
    neighbor = neighbor_index == -1 ? n->parent->pointers[1] : 
        n->parent->pointers[neighbor_index];

    capacity = n->is_leaf ? order : order - 1;

    /* Coalescence. */

    if (neighbor->num_keys + n->num_keys < capacity)
        return coalesce_nodes(root, n, neighbor, neighbor_index, k_prime);

    /* Redistribution. */

    else
        return redistribute_nodes(root, n, neighbor, neighbor_index, k_prime_index, k_prime);
}



/* Master deletion function.
 */
node * delete_record(node * root, uint64_t key) {

    node * key_leaf;
    record_t * key_record_t;

    key_record_t = find(root, key, false);
    key_leaf = find_leaf(root, key, false);
    if (key_record_t != NULL && key_leaf != NULL) {
        root = delete_entry(root, key_leaf, key, key_record_t);
        free(key_record_t);
    }
    return root;
}


void destroy_tree_nodes(node * root) {
    int i;
    if (root->is_leaf)
        for (i = 0; i < root->num_keys; i++)
            free(root->pointers[i]);
    else
        for (i = 0; i < root->num_keys + 1; i++)
            destroy_tree_nodes(root->pointers[i]);
    free(root->pointers);
    free(root->keys);
    free(root);
}


node * destroy_tree(node * root) {
    destroy_tree_nodes(root);
    return NULL;
}

#endif

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(){
    static page_t buf;
    if(fd <= 0)
        return FILEDESCRIPTOR_ERROR;

    // 만약 free page가 없다면, DEFAULT_SIZE_OF_FREE_PAGES크기 만큼 free page list 만들기
    if(header.free_page_offset==0){
        // Write a few of free pages
        CLEAR(buf);
        int i;
        for(i=1; i<=DEFAULT_SIZE_OF_FREE_PAGES; ++i){            
            buf.next_free_page_offset = ( ( i + 1 ) % ( DEFAULT_SIZE_OF_FREE_PAGES + 1 ) ) * PAGESIZE;
            if(!WRITE(buf)) {
                perror("Not enough memory");
                return -1;
            }
        }
        SEEK(HEADER_OFFSET);
        READ(header);
        ;// 새로운 free page를 요청한다.
    }

    // Free page로 이동한다.
    if(!SEEK(header.free_page_offset)){
        perror("File seek error");
        return PAGE_NOT_FOUND;
    }

    // free page 하나를 읽어온다.
    if(!READ(buf)){
        perror("File read error");
        return PAGE_READ_ERROR;
    }
     
    // free page를 free page list에서 분리한다.
    header.free_page_offset = buf.next_free_page_offset;

    // header free page를 갱신한다.
    SEEK(HEADER_OFFSET);
    WRITE(header);
    return header.free_page_offset;
}

// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum){
    if(fd <= 0)
        return;

    if(!SEEK(OFFSET(pagenum))){
        perror("File seek error");
        return;
    }

    page_t buf;
    READ(buf);

    // Make it a free page
    CLEAR(buf);

    // Add it into the free page list
    buf.free_page.next_free_page_offset = header.hd.free_page_offset;
    header.hd.free_page_offset = OFFSET(pagenum);

    // Apply changes to the header page.
    SEEK(0);
    WRITE(header);    
}

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest){
    SEEK(OFFSET(pagenum));
    READ(*dest);
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src){
    SEEK(OFFSET(pagenum));
    WRITE(*src);
}

/*
    Open existing data file using ‘pathname’ or create one if not existed.
    If success, return 0. Otherwise, return non-zero value.
*/
int open_db (char *pathname){
    // 파일 이름이 이상하면 터트린다.
    if(pathname == NULL) return -1;
    // 파일을 없으면 만든다 있으면 그 파일을 연다.
    if(fd == 0){
        // Make a file if it does not exist.
        // Do not use a symbolic link.
        // Flush the buffer whenever try to write.
        // Permission is set to 0644.
        fd = open(pathname, O_CREAT | O_RDWR | O_NOFOLLOW | O_SYNC, S_IWUSR | S_IWUSR | S_IRGRP | S_IROTH);
        // Add a handler which is to be executed before the process exits.
        atexit(when_exit);
    }
    // 파일 출력에 실패하면, 터트린다.
    if(fd == -1){
        perror("open_db error");
        return -1;
    }
    // Start to read from the header
    if(READ(header) == 0){
        // If the header page doesn't exist, create a new one.

        // Write a header page.
        CLEAR(header);
        header.free_page_offset = PAGESIZE;
        header.root_page_offset = 0; // 0 means the root page does not exist.
        header.num_of_page = 1; // the number of created pages
        
        if(!WRITE(header)) {
            perror("Not enough memory");
            return -1;
        }
    }
    /*
    printf("number of pages: %ld\n", buf.hd.num_of_page);
    printf("free page offset: %ld\n", buf.hd.free_page_offset);
    offset_t offset = buf.hd.free_page_offset;
    do{
        if(lseek(fd, offset, SEEK_SET)<0){
            perror("File seek error");
            break;
        }
        if(read(fd, &buf, sizeof buf) < 0){
            perror("File Read error");
            break;   
        }
        printf("free page found\t offset: %ld\n", offset);
    }while((offset=buf.free_page.next_free_page_offset)!=0);
    */
    return 0;
}

/*
    Find the matching record_t and delete it if found.
    If success, return 0. Otherwise, return non-zero value.
*/

int delete (int64_t key){
    // 키를 검색한다.
    // 키가 있다면, 해당 키를 제거하고, 0을 return한다..
    // 키가 없다면, 0이 아닌 값을 return한다. 
}

void when_exit(void){
    fsync(fd);
    if(fd!=0)
        close(fd);
}