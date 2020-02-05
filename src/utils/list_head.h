/*******************************************************************************
Copyright (c) 2006, Nikolay Nosov
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, this 
  list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this 
  list of conditions and the following disclaimer in the documentation and/or 
  other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#ifndef __LIST_HEAD_H__
#define __LIST_HEAD_H__

/**
Double linked list for user space implementation
*/

#include <stddef.h> /* for NULL declaration */

/** Get offset of a member
@param TYPE     the type of the struct
@param MEMBER   the name of the member, which offset to be computed
*/
#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

/** Casts a member of a structure out to the containing structure
 @param ptr     the pointer to the member.
 @param type    the type of the container struct this is embedded in
 @param member  the name of the member within the struct.
*/

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type, member) );})

/** Get the struct for this entry
 @param ptr     the &struct list_head pointer.
 @param type    the type of the struct this is embedded in.
 @param member  the name of the list_struct within the struct
*/

#define list_entry(ptr, type, member) container_of(ptr, type, member)

/** Iterate over a list
 @param pos     the &struct list_head to use as a loop counter.
 @param head    the head for your list.
*/

#define list_for_each(pos, head) \
	for (pos = (head)->first; pos; \
	     pos = pos->next)

/**
 * list_for_each_entry - iterate over list of given type
 * @tpos:       the type * to use as a loop cursor.
 * @pos:        the &struct hlist_node to use as a loop cursor.
 * @head:       the head for your list.
 * @member:     the name of the hlist_node within the struct.
 */
#define list_for_each_entry(tpos, pos, head, member)                    \
        for (pos = (head)->first;                                        \
             pos && ({ tpos = list_entry(pos, typeof(*tpos), member); 1;}); \
             pos = pos->next)


/** Iterate over list of given type safe against removal of list entry
 * @tpos:       the type * to use as a loop cursor.
 * @pos:        the &struct list_node to use as a loop cursor.
 * @n:          another &struct list_node to use as temporary storage
 * @head:       the head for your list.
 * @member:     the name of the list_node within the struct.
*/
#define list_for_each_entry_safe(tpos, pos, n, head, member)            \
        for (pos = (head)->first;                                        \
             pos && ({ n = pos->next; 1; }) &&                           \
                ({ tpos = list_entry(pos, typeof(*tpos), member); 1;}); \
             pos = n)


/** Iterate over a list backwards
 @param pos     the &struct list_head to use as a loop counter.
 @param head    the head for your list.
*/

#define list_for_each_prev(pos, head) \
	for (pos = (head)->last; pos; \
        	pos = pos->prev)


/** Iterate over a list safe against removal of list entry
 @param pos     the &struct list_head to use as a loop counter.
 @param n       another &struct list_head to use as temporary storage
 @param head    the head for your list.
*/

#if !defined(list_for_each_safe)
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->first; pos && ({ n = pos->next; 1; }); \
	     pos = n)
#endif

/* to compiler be happy */

struct list_head;

/** Node of the list
@param next     next node in the list
@param prev     previous node in the list
@param head     head of the list
*/

typedef struct list_node
{
    struct list_node *next, *prev;
    struct list_head *head;
} list_node;



/** Head of the list
@param first    first node in the list
@param last     last node in the list
@param count    cont of the nodes
*/

typedef struct list_head
{
    struct list_node *first, *last;
    int count;
} list_head;

/**
Compile time initialization of the head struct
*/

#define LIST_HEAD_INIT { .first = NULL, .last = NULL, .count = 0 }

/**
Compile time initialization of the node struct
*/

#define LIST_NODE_INIT { .prev = NULL, .next = NULL, .head = NULL }

/** Runtime initialization of the head struct
(should be applied before first using of the list)
@param node     head, which to be initialized.
*/

static inline void list_head_init(struct list_head *head)
{
    head->first = head->last = NULL;
    head->count = 0;
}


/** Runtime initialization of the node struct
(should be applied before first using of the node)
@param node     node, which to be initialized.
*/

static inline void list_node_init(struct list_node *node)
{
    node->next = node->prev = NULL;
    node->head = NULL;
}

/** Add node to begin of the list
@param head     the head of the list
@param node     node of the list, which to be added
*/

static inline void list_add_head(struct list_head *head, struct list_node *node)
{
    struct list_node *pfirst = head->first;
    head->first = node;
    node->next = pfirst;
    node->prev = NULL;
    node->head = head;

    if(pfirst) pfirst->prev = node;
        else head->last = node;

    head->count++;
}

/** Add node to the end of the list
@param head     the head of the list;
@param node     node of the list, which to be added
*/

static inline void list_add_bottom(struct list_head *head, struct list_node *node)
{
    struct list_node *plast = head->last;

    head->last = node;
    node->next = NULL;
    node->prev = plast;
    node->head = head;

    if(plast) plast->next = node;
        else head->first = node;

    head->count++;
}

/** Add node in the list after given 'base' node
@param base     the base node, after that node will be added
@param node     node of the list, which to be added
*/

static inline void list_add_after(struct list_node *base, struct list_node *node)
{
    struct list_node *pnext = base->next;

    base->next = node;
    node->next = pnext;
    node->prev = base;
    node->head = base->head;

    if(pnext) pnext->prev = node;
        else base->head->last = node;

    base->head->count++;
}

/** Adds node in the list before given 'base' node
@param base     the base node, before that node will be added
@param node     node of the list, which to be added
*/

static inline void list_add_before(struct list_node *base, struct list_node *node)
{
    struct list_node *pprev = base->prev;

    base->prev = node;
    node->next = base;
    node->prev = pprev;
    node->head = base->head;

    if(pprev) pprev->next = node;
        else base->head->first = node;
    base->head->count++;
}

/** Remove node from the list
@param node     the node, which to be removed
*/

static inline void list_del(struct list_node *node)
{
    if(node->head)
    {
        if(node->prev) node->prev->next = node->next;
            else node->head->first = node->next;
        if(node->next) node->next->prev = node->prev;
            else node->head->last = node->prev;
        node->head->count--;
    }

    node->prev = node->next = NULL;
    node->head = NULL;
}

static inline void list_del_init(struct list_node *node)
{
    list_del(node);
    list_node_init(node);
}

/** Count of the nodes in the list
@param head     the head of the list
@return count of the nodes in the list
*/

static inline int list_count(struct list_head *head)
{
    return head->count;
}

/** First element of the list
@param head     the head of the list
@return pointer to first element of the list
*/

static inline struct list_node* list_first(struct list_head *head)
{
    return head->first;
}

/** Last element of the list
@param head     the head of the list
@return pointer to last element of the list
*/

static inline struct list_node* list_last(struct list_head *head)
{
    return head->last;
}

/** Next element of the list
@param head     current element of the list
@return pointer to next element relatively of current
*/

static inline struct list_node* list_next(struct list_node *node)
{
    return node->next;
}

/** Previous element of the list
@param head     current element of the list
@return pointer to previous element relatively of current
*/

static inline struct list_node* list_prev(struct list_node *node)
{
    return node->prev;
}

#endif // __LIST_HEAD_H__

