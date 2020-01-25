/*
 * dictionary.cpp
 *
 *  Created on: Oct 28, 2014
 *      Author: nnosov
 */
#include "dictionary.h"

//framework
#include "logger/log.h"

extern SC_LogBase* logger;

typedef struct __key_value {
    const char* key;
    uint32_t    keylen;
    int         value;
} key_value;

#define KEY_VALUE(key, value)  {key, sizeof(key), value}

key_value   utable[] = {
        KEY_VALUE("key1", 1),
        KEY_VALUE("key2", 2),
        KEY_VALUE("key1", 3),
};


bool SC_Dict::UnitTest()
{
    logger->Log(SEVERITY_DEBUG, "%s: starting ...", __PRETTY_FUNCTION__);
    logger->Log(SEVERITY_DEBUG, "\t\tshift = %d, hash function = %p, number of buckets = %d, mask = 0x%X. count = %d", mShift, pHashFn, mSize, mMask, mCount);
    logger->Log(SEVERITY_DEBUG, "... inserting values into the dictionary ...");
    for(uint32_t i = 0; i < sizeof(utable) / sizeof(key_value); i++)
    {
        logger->Log(SEVERITY_DEBUG, "\t\tKey = %s, Key size = %d, Key value = %d", utable[i].key, utable[i].keylen, utable[i].value);
        Insert(utable[i].key, utable[i].keylen, &(utable[i].value));
    }
    logger->Log(SEVERITY_DEBUG, "\t\tdictionary size = %d",mCount);
    logger->Log(SEVERITY_DEBUG, "... went through the dictionary sequentially ...");
    for(void* data = First(); data; data = Next())
    {
        logger->Log(SEVERITY_DEBUG, "\t\tvalue = %d", *((int*)data));
    }

    logger->Log(SEVERITY_DEBUG, "... went through the dictionary in key order ...");
    for(uint32_t i = 0; i < sizeof(utable) / sizeof(key_value); i++)
    {
        void* data = Lookup(utable[i].key, utable[i].keylen);
        if(data)
            logger->Log(SEVERITY_DEBUG, "\t\tKey = %s, Key size = %d, Key value = %d", utable[i].key, utable[i].keylen, *((int*)data));
    }

    void* data = Lookup(utable[0].key, utable[0].keylen);
    if(data)
    {
        logger->Log(SEVERITY_DEBUG, "... removing first key[%s] ...", utable[0].key);
        Remove();
    }

    logger->Log(SEVERITY_DEBUG, "... went through the dictionary in key order 2 ...");
    for(uint32_t i = 0; i < sizeof(utable) / sizeof(key_value); i++)
    {
        void* data = Lookup(utable[i].key, utable[i].keylen);
        if(data)
            logger->Log(SEVERITY_DEBUG, "\t\tKey = %s, Key size = %d, Key value = %d", utable[i].key, utable[i].keylen, *((int*)data));
    }

    logger->Log(SEVERITY_DEBUG, "%s: finished.", __PRETTY_FUNCTION__);
    return true;
}
