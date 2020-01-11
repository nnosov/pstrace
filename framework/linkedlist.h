/*
 * linkedlist.h
 *
 *  Created on: Oct 24, 2013
 *      Author: nnosov
 *
 *  Simple double linked list implementation. Base class.
 */

#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_

//system
#include <stdint.h>

typedef struct __SC_ListNode
{
    __SC_ListNode()
    {
        pPrev = pNext = 0;
    }

    bool IsListed()
    {
        return (pNext || pPrev);
    }

    __SC_ListNode*    pNext;
    __SC_ListNode*    pPrev;
} SC_ListNode;


class SC_ListHead
{

public:
    SC_ListHead()
    {
        mHead.pPrev = mHead.pNext = &mHead;
        size = 0;
    }

    virtual ~SC_ListHead()
    {
    	Clear();
    }

    void Clear()
    {
    	for(SC_ListNode* node = (SC_ListNode*)First(); node; node = (SC_ListNode*)First())
    	{
    		Remove(node);
    	}
    }

    bool IsEmpty() const
    {
        return (mHead.pNext == &mHead);
    }

    SC_ListNode* First()
    {
        return IsEmpty() ? 0 : mHead.pNext;
    }

    SC_ListNode* Last()
    {
        return IsEmpty() ? 0 : mHead.pPrev;
    }

    SC_ListNode* Next(SC_ListNode* node)
    {
        return (!node || node == Last() || !node->IsListed()) ? 0 : node->pNext;
    }

    SC_ListNode* Prev(SC_ListNode* node)
    {
        return (!node || node == First() || !node->IsListed()) ? 0 : node->pPrev;
    }

    void InsertFirst(SC_ListNode* node)
    {
        node->pNext = mHead.pNext;
        mHead.pNext = node;
        node->pNext->pPrev = node;
        node->pPrev = &mHead;

        size++;
    }

    void InsertLast(SC_ListNode* node)
    {
        node->pNext = &mHead;
        node->pPrev = mHead.pPrev;
        node->pPrev->pNext = node;
        mHead.pPrev = node;

        size++;
    }

    void InsertAfter(SC_ListNode* pPrev, SC_ListNode* p)
    {
    	p->pNext = pPrev->pNext;
    	p->pPrev = p->pNext->pPrev;
    	p->pNext->pPrev = p;
    	p->pPrev->pNext = p;
    	size++;
    }

    void InsertBefore(SC_ListNode* pNext, SC_ListNode* p)
    {
    	p->pNext = pNext;
    	p->pPrev = pNext->pPrev;
    	pNext->pPrev = p;
    	p->pPrev->pNext = p;
    	size++;
    }

    void Remove(SC_ListNode* node)
    {
        node->pPrev->pNext = node->pNext;
        node->pNext->pPrev = node->pPrev;
        node->pNext = node->pPrev = 0;

        size--;
    }

    uint32_t Size()
    {
        return size;
    }

protected:
    SC_ListNode        mHead;   //previous element in list
    uint32_t        size;   //total number of elements in list
};

#endif /* LINKEDLIST_H_ */
