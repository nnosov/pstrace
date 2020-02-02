#ifndef HASH_MULTIMAP_H
#define HASH_MULTIMAP_H

/* =============================================================================
 * CDL (Configuration Definition Language) validator $Revision: 1.4 $
 * (C)2004-2007 Nikolai Nosov. All rights reserved.
 *
 * File:            $RCSfile: hash_multimap.c,v $
 * Purpose:         Hash Multimap implementation
 * Written by:      Nikolai Nosov nnosov@gmail.com
 * Last modified:   $Date: 2008/06/21 12:15:53 $ by $Author: nnosov $.
 *
 * For more information please visit
 * http://cdl.sourceforge.net
 * ===========================================================================*/

#include <stdint.h>

#include "list_head.h"

#define HASH_MIN_SHIFT (8)
#define HASH_MAX_SHIFT (15)

#define hash_entry(ptr, type, member) container_of(ptr, type, member)

/** Hash multimap node descriptor

@param key hash key value
@param key_size hash key size in bytes
@param node uplink to hash multimap table
*/
typedef struct hash_node
{
    list_node   node;
    char*       key;
    int         key_size;
} hash_node;

/** Hash function prototype

@param key hash key value
@param key_size hash key size in bytes
@return hash value
*/
typedef unsigned int (* _hash_fn)(const char *key, int key_size);

/** Compare function prototype
 * 
 * @param key1 pointer to the first key for comparison
 * @param key2 pointer to the second key for comparison
 * @param size size of compared part of the keys in bytes
 * 
 * @return zero if keys equals each other, othervize 1
*/
typedef int (*_compare_fn)(const char *key1, const char *key2, int size);

/** Hash multimap table descriptor

@param hash_shift size of hash table as power of 2
@param hash_size size of the hash table
@param hash_mask mask for the hash value
@param bucket pointer to the array of linked lists. in other words, pointer to
hash multimap table
@param count count of the nodes in the hash table
*/
typedef struct hash_head {
    uint8_t       hash_shift;
    uint16_t      hash_size;
    uint32_t      hash_mask;
    list_head*    bucket;

    _hash_fn      hash_fn;
    _compare_fn   compare_fn;
} hash_head;

/** Hash table iterator

@param map pointer to the hash table descriptor, used for iteration
@param current pointer to the current list node (bucket[map_idx])
@param map_idx current bucket index
*/
struct hash_iterator {
    hash_head*      map;       // pointer to the hash map header
    list_node*      current;   // pointer to current list node in the bucket[map_idx] list
    int             map_idx;   // index of current list in the hash map bucket
};

void hash_node_init(struct hash_node *node);
void hash_node_cleanup(struct hash_node *node);

unsigned int default_hash_fn(const char *key, int size);
int default_compare_fn(const char *key1, const char *key2, const int size);

int hash_head_init(struct hash_head *map, unsigned int hash_shift = 8, _hash_fn hf = default_hash_fn, _compare_fn cf = default_compare_fn);
void hash_head_cleanup(struct hash_head *map);

struct hash_node* hash_find(struct hash_head *map, const void *key, int key_size);
struct hash_node* hash_find_next(struct hash_head *map, struct hash_node *node);

int hash_add(struct hash_head *map, struct hash_node *node, void *key, int key_size);
void hash_del(struct hash_node *node);

void hash_iterator_init(struct hash_head *map, struct hash_iterator *iter);
struct hash_node * hash_node_first(struct hash_iterator *iter);
struct hash_node * hash_node_next(struct hash_iterator *iter);
void hash_node_del(struct hash_iterator *iter);
int hash_count(struct hash_head *head);

#endif // HASH_MULTIMAP_H
