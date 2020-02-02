/* =============================================================================
 * CDL (Configuration Definition Language) validator $Revision: 1.4 $
 * (C)2004-2007 Nikolay Nosov. All rights reserved.
 *
 * File:            $RCSfile: hash_multimap.c,v $
 * Purpose:         Hash Multimap implementation
 * Written by:      Nikolay Nosov
 * Last modified:   $Date: 2008/06/21 12:15:53 $ by $Author: nnosov $.
 *
 * For more information please visit
 * http://nsoft.volgocity.ru/cdl or
 * http://cdl.sourceforge.net
 * ===========================================================================*/
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "hash_multimap.h"

/* Default hash function
@param key hash key
@param size hash key size in bytes
@return hash value, depends of the hash key
*/
static unsigned int default_hash_fn(const void *key, int size)
{
    register int i, j;
    unsigned int cs = 0;
    unsigned int ps = 0;

    if(key)
    {
        for(i = 0, j = 0; i < size; i++, j++)
        {
            ps |= ((unsigned int)((const char*)key)[i]) << (j * 8);
            if(j == 3 || j == (size - 1))
            {
                cs ^= ps;
                ps = 0;
                j = 0;
            }
        }
    }

    return cs;
}

static int default_compare_fn(const void *key1, const void *key2, const int size)
{
    return !memcmp(key1, key2, size);
}

/** Initialize hash node. Must be called before first usage of the node.

@param node pointer to the hash node descriptor
*/
void hash_node_init(struct hash_node *node)
{
    node->key = NULL;
    node->key_size = 0;
    list_node_init(&node->node);
}

/** Cleanup hash node

@param node pointer to the hash node descriptor
*/
void hash_node_cleanup(struct hash_node *node)
{
    if(node->key) free(node->key);
    list_node_init(&node->node);
    node->key = NULL;
    node->key_size = 0;
}

/** Initialize hash table

@param map pointer to the hash table descriptor
@param hash_shift size of the hash table in bits. Should be greater than
HASH_MIN_SHIFT and less than HASH_MAX_SHIFT. In case of less than HASH_MIN_SHIFT,
will be followed by HASH_MIN_SHIFT, in case of greater HASH_MAX_SHIFT will be
followed by HASH_MAX_SHIFT
@param hf pointer to the user defined hash function. If NULL, then default hash
function will be used
@param cf pointer to the user defined compare function. if NULL, then the default
compare function will be used
*/
int hash_head_init(struct hash_head *map, unsigned int hash_shift, _hash_fn hf, _compare_fn cf)
{
    int i;

    if(map)
    {
        if(hash_shift <= HASH_MIN_SHIFT) {
            map->hash_shift = HASH_MIN_SHIFT;
        } else if(hash_shift >= HASH_MAX_SHIFT) {
            map->hash_shift = HASH_MAX_SHIFT;
        } else {
            map->hash_shift = hash_shift;
        }

        map->hash_size = 1UL << map->hash_shift;
        map->hash_mask = (map->hash_size - 1);

        map->bucket = (struct list_head *)malloc(sizeof(struct list_head) * map->hash_size);
        if(map->bucket)
        {
            for(i = 0; i < map->hash_size; i++)
            {
                list_head_init(map->bucket + i);
            }

            if(hf) {
                map->hash_fn = hf;
            } else {
                map->hash_fn = default_hash_fn;
            }

            if(cf) {
                map->compare_fn = cf;
            } else {
                map->compare_fn = default_compare_fn;
            }
        }
        else
            return ENOMEM;
    }
    else
        return ENODEV;

    return 0;
}

/** Cleanup hash table descriptor

@param map pointer to the hash table descriptor
*/
void hash_head_cleanup(struct hash_head *map)
{
    struct list_node    *p, *n;
    struct hash_node *node;
    int                 i;

    if(map)
    {
        if(map->bucket)
        {
            for(i = 0; i < map->hash_size; i++)
            {
                list_for_each_safe(p, n, map->bucket + i)
                {
                    node = list_entry(p, struct hash_node, node);
                    hash_node_cleanup(node);
                }
            }
            free(map->bucket);
        }
    }
}

/** Find first node which key is equal to the key represented

@param map pointer to the hash table descriptor
@param key pointer to the hash key
@param key_size size of the hash key
@return pointer to the key found, or NULL in case of hash table have not nodes
with the key equals to the key represented
*/
struct hash_node* hash_find(struct hash_head *map, const void *key, int key_size)
{
    struct list_head    *list;
    struct list_node    *n;
    struct hash_node    *node;

    list = map->bucket + (map->hash_fn(key, key_size) & map->hash_mask);
    list_for_each(n, list)
    {
        node = list_entry(n, struct hash_node, node);
        if(key_size == node->key_size)
        {
            if(map->compare_fn(key, node->key, key_size)) return node;
        }
    }

    return NULL;
}

/** Find next node, which key is equal to the key represented

@param hn1 pointer to the previous node found.
@return pointer to the next node with the same key, or NULL in case of the current
node is the last node with appropriate key
*/
struct hash_node* hash_find_next(struct hash_head *map, struct hash_node *hn1)
{
    struct list_node    *n = &hn1->node;
    struct hash_node *hn2;


    while((n = list_next(n)) != NULL)
    {
        hn2 = list_entry(n, struct hash_node, node);
        if(hn1->key_size == hn2->key_size)
        {
            if(map->compare_fn(hn1->key, hn2->key, hn2->key_size)) return hn2;
        }
    }

    return NULL;
}

/** Add node to the hash table

@param map pointer to the hash table
@param node pointer to the node, which will be added to the table
@param key pointer to the hash key
@param key_size size of the hash key

@return zero in case of success, ENOMEM in case of error
*/
int hash_add(struct hash_head *map, struct hash_node *node, const void *key, int key_size)
{
    node->key = (char*)malloc(key_size);
    if(node->key)
    {
        memcpy(node->key, key, key_size);
        node->key_size = key_size;
        list_add_bottom(map->bucket + (map->hash_fn(key, key_size) & map->hash_mask), &node->node);

        return 0;
    }

    return ENOMEM;
}

/** Remove node from the hash table

@param node pointer to the node descriptor
*/
void hash_del(struct hash_node *node)
{
    list_del_init(&node->node);
    hash_node_cleanup(node);
}

/** Initialize iterator for usage. Must be called before first iterator usage.

@param map pointer to the hash table descriptor
@param iter pointer to the iterator descriptor
*/
void hash_iterator_init(struct hash_head *map, struct hash_iterator *iter)
{
    iter->map = map;
    iter->map_idx = 0;
    iter->current = NULL;
}

/** Move iterator to the first node in the table, and return pointer to the node

@param iter pointer to the iterator descriptor
@return pointer to the first node in the table, or NULL in case of error
*/
struct hash_node * hash_node_first(struct hash_iterator *iter)
{
    iter->current = list_first(iter->map->bucket);
    iter->map_idx = 0;

    if (iter->current)
    {
        return list_entry(iter->current, struct hash_node, node);
    }

    return NULL;
}

/** Move iterator to the next node in the table and return pointer to the node

@param iter pointer to the iterator descriptor
@return pointer to the next node in the table, or NULL in case of error
*/
struct hash_node * hash_node_next(struct hash_iterator *iter)
{
    if(iter->map_idx >= iter->map->hash_size) return NULL;

    if(iter->current) {
        iter->current = list_next(iter->current);
    }
    if(iter->current) {
        return list_entry(iter->current, struct hash_node, node);
    }

    while(iter->map_idx < iter->map->hash_size) {
        iter->current = list_first(iter->map->bucket + iter->map_idx);
        iter->map_idx++;
        if(iter->current) {
            return list_entry(iter->current, struct hash_node, node);
        }
    }

    return NULL;
}

/** Remove current node, pointed by iterator, and move iterator to the next node

@param iter pointer to the iterator descriptor
*/
void hash_node_del(struct hash_iterator *iter)
{
    struct list_node *tmp;

    tmp = iter->current;
    hash_node_next(iter);
    list_del_init(tmp);
}

/** Return total count of elements in the hash table

@param head pointer to the hash table descriptor
*/
int hash_count(struct hash_head *head)
{
    int i;
    int count = 0;

    for(i = 0; i < head->hash_size; i++) count += list_count(head->bucket + i);

    return count;
}
