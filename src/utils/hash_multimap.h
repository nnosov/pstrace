#ifndef HASH_MULTIMAP_H
#define HASH_MULTIMAP_H

#include "../src/utils/list_head.h"

#define HASH_MIN_SHIFT (8)
#define HASH_MAX_SHIFT (15)

#define hash_entry(ptr, type, member) container_of(ptr, type, member)

/** Hash multimap node descriptor

@param key hash key value
@param key_size hash key size in bytes
@param node uplink to hash multimap table
*/
struct hash_node
{
    char                *key;
    int                 key_size;
    struct list_node    node;
};

/** Hash function prototype

@param key hash key value
@param key_size hash key size in bytes
@return hash value
*/
typedef unsigned int (* _hash_fn)(const void *key, int key_size);

/** Compare function prototype
 * 
 * @param key1 pointer to the first key for comparison
 * @param key2 pointer to the second key for comparison
 * @param size size of compared part of the keys in bytes
 * 
 * @return zero if keys equals each other, othervize 1
*/
typedef int (*_compare_fn)(const void *key1, const void *key2, int size);

/** Hash multimap table descriptor

@param hash_shift size of hash table as power of 2
@param hash_size size of the hash table
@param hash_mask mask for the hash value
@param bucket pointer to the array of linked lists. in other words, pointer to
hash multimap table
@param count count of the nodes in the hash table
*/
struct hash_head
{
    unsigned char       hash_shift;
    unsigned short      hash_size;
    unsigned int        hash_mask;
    struct list_head    *bucket;
    //int                 count;

    _hash_fn            hash_fn;
    _compare_fn         compare_fn;
};

/** Hash table iterator

@param map pointer to the hash table descriptor, used for iteration
@param current pointer to the current list node (bucket[map_idx])
@param map_idx current bucket index
*/
struct hash_iterator
{
    struct hash_head    *map;       // pointer to the hash map header
    struct list_node    *current;   // pointer to current list node in the bucket[map_idx] list
    unsigned int        map_idx;    // index of current list in the hash map bucket
};

void hash_node_init(struct hash_node *node);
void hash_node_cleanup(struct hash_node *node);

int hash_head_init(struct hash_head *map, unsigned int hash_shift = HASH_MIN_SHIFT, _hash_fn hf = 0, _compare_fn cf = 0);
void hash_head_cleanup(struct hash_head *map);

struct hash_node* hash_find(struct hash_head *map, const void *key, int key_size);
struct hash_node* hash_find_next(struct hash_head *map, struct hash_node *node);

int hash_add(struct hash_head *map, struct hash_node *node, const void *key, int key_size);
void hash_del(struct hash_node *node);

void hash_iterator_init(struct hash_head *map, struct hash_iterator *iter);
struct hash_node * hash_node_first(struct hash_iterator *iter);
struct hash_node * hash_node_next(struct hash_iterator *iter);
void hash_node_del(struct hash_iterator *iter);
int hash_count(struct hash_head *head);

#endif // HASH_MULTIMAP_H
