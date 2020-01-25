/*
 * dictionary.h
 *
 *  Created on: Aug 5, 2014
 *      Author: nnosov
 */

#ifndef DICTIONARY_H_
#define DICTIONARY_H_

//system
#include <stdint.h>
#include <string.h>

//framework
#include "data_entry.h"
#include "linkedlist.h"

#define HASH_MIN_SHIFT (8)
#define HASH_MAX_SHIFT (15)

typedef uint32_t (* SC_HashFn)(const void *key, uint32_t key_size);

inline uint32_t hash_bernstein(const void* key, uint32_t key_size)
{
        uint32_t hash = 0;
        register unsigned char* p = (unsigned char*)key;

        for (unsigned int i = 0; i < key_size; i++)
        {
            hash = 33 * hash + p[i];
        }

        return hash;
}

inline uint32_t hash_sdbm(const void *key, uint32_t key_size)
{
    uint32_t hash = 0;
    register unsigned char* p = (unsigned char*)key;

    for (uint32_t i = 0; i < key_size; i++)
    {
        hash = p[i] + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

inline uint32_t hash_fnv(const void *key, uint32_t key_size )
{
    register unsigned char *p = (unsigned char*)key;
    uint32_t hash = 2166136261U;

    for (uint32_t i = 0; i < key_size; i++ )
    {
        hash = ( hash * 16777619 ) ^ p[i];
    }

    return hash;
}

typedef struct __SC_DictItem : public SC_ListNode
{
    __SC_DictItem()
    {
        data = 0;
        hash = 0;
    }

    ~__SC_DictItem()
    {
        if(key.data) {
            delete[] key.data;
        }
        key.data = 0;
        key.size = 0;
    }

    void*       data;
    SC_Data     key;
    uint32_t    hash;
} SC_DictItem;

class SC_Dict
{
private:
    SC_Dict(SC_Dict& other) : pHashFn(0), mCurIdx(0), pCurItem(0), mCount(0), mShift(0), mMask(0), mBuckets(0), mSize(0){(void) other;}

public:
    SC_Dict(uint16_t shift = 8, SC_HashFn fn = hash_bernstein) : pHashFn(fn), mCurIdx(0), pCurItem(0), mCount(0)
    {
        if(shift <= HASH_MIN_SHIFT) {
            mShift = HASH_MIN_SHIFT;
        } else if(shift >= HASH_MAX_SHIFT) {
            mShift = HASH_MAX_SHIFT;
        } else {
            mShift = shift;
        }

        mSize = 1UL << shift;
        mMask = mSize -1;

        mBuckets = new SC_ListHead[mSize];
    }

    virtual ~SC_Dict()
        {
        	Clean();
        	delete[] mBuckets;
        }

        void Clean()
        {
        	First();
        	while(mCount) Remove();
        	mCurIdx = 0;
        	pCurItem = 0;
        }


    inline void Insert(const void* key, uint32_t size, void* data)
    {
        SC_DictItem* item = new SC_DictItem;
        item->hash = pHashFn(key, size);
        item->data = data;

        // save key
        item->key.data = new char[size];
        memcpy(item->key.data, key, size);
        item->key.size = size;

        mCurIdx = item->hash & mMask;

        for(pCurItem = (SC_DictItem*)mBuckets[mCurIdx].First(); pCurItem ; pCurItem = (SC_DictItem*)mBuckets[mCurIdx].Next(pCurItem))
        {
            if(pCurItem->key.size == item->key.size)
            {
                if(!memcmp(pCurItem->key.data, item->key.data, size))
                {
                    pCurItem->data = data;
                    delete item;
                    return;
                }
            }
        }
        mBuckets[mCurIdx].InsertLast(item);
        pCurItem = item;
        mCount++;
    }

    void Remove()
    {
        SC_DictItem* item = pCurItem;
        if(item)
        {
            MoveNext();
            mBuckets[item->hash & mMask].Remove(item);
            if(pCurItem == item)
                pCurItem = 0;
            delete item;
            mCount--;
        }
    }

    //returns first data in dictionary
    inline void* First()
    {
        if(!mCount)
        {
            return 0;
        }

        mCurIdx = 0;
        pCurItem = 0;
        MoveNext();

        return pCurItem ? pCurItem->data : 0;
    }

    //returns next data in dictionary
    inline void* Next()
    {
        MoveNext();
        return pCurItem ? pCurItem->data : 0;
    }

    //looking up for the next data
    inline void* LookupNext(const void* key, uint32_t size)
    {
        uint32_t hash = pHashFn(key, size);

        for(SC_DictItem* curr = (SC_DictItem*)mBuckets[mCurIdx].Next(pCurItem); curr; curr = (SC_DictItem*)mBuckets[mCurIdx].Next(curr))
        {
            if(curr->hash == hash && curr->key.size == size)
            {
                if(!memcmp(key, curr->key.data, size))
                {
                    pCurItem = curr;
                    return curr->data;
                }
            }
        }

        return 0;
    }

    //looking up for the first data corresponds to the key value
    inline void* Lookup(const void* key, uint32_t size)
    {
        uint32_t hash = pHashFn(key, size);
        uint32_t idx = hash & mMask;
        for(SC_DictItem* curr = (SC_DictItem*)mBuckets[idx].First(); curr; curr = (SC_DictItem*)mBuckets[idx].Next(curr))
        {
            if(curr->hash == hash && curr->key.size == size)
            {
                if(!memcmp(key, curr->key.data, size))
                {
                    mCurIdx = idx;
                    pCurItem = curr;
                    return curr->data;
                }
            }
        }

        return 0;
    }

    uint32_t Size()
    {
       return mCount;
    }

protected:
    inline SC_DictItem* MoveNext()
    {
        if(mCurIdx >= mSize)
        {
            return 0;
        }

        if(pCurItem)
        {
            pCurItem = (SC_DictItem*)mBuckets[mCurIdx].Next(pCurItem);
            if(!pCurItem)
                mCurIdx++;
        }

        while(!pCurItem && mCurIdx < mSize)
        {
            pCurItem = (SC_DictItem*)mBuckets[mCurIdx].First();
            if(pCurItem)
                break;
            mCurIdx++;
        }

        return pCurItem;
    }
public:
    virtual bool UnitTest();

protected:

    SC_HashFn       pHashFn;    //hash function
    uint32_t        mCurIdx;    //index of current bucket
    SC_DictItem*    pCurItem;   //current item
    uint32_t        mCount;     //number of elements in dictionary
    uint32_t        mShift;     //shift of the hash
    uint32_t        mMask;      //mask to be applied to the hash
    SC_ListHead*    mBuckets;   //Buckets of hash table
    uint32_t        mSize;      //size of array in buckets
};


#endif /* DICTIONARY_H_ */
