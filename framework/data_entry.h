/*
 * data_entry.h
 *
 *  Created on: Oct 30, 2013
 *      Author: nnosov
 *
 *  Simple representation of data. Pointer to data and size of data.
 */

#ifndef DATA_ENTRY_H_
#define DATA_ENTRY_H_


//system
#include <stdint.h>

typedef struct _SC_Data
{
    _SC_Data()
    {
        data = 0;
        size =0;
    }

    _SC_Data(const char* _data, uint32_t _size)
    {
        data = (char*)_data;
        size = _size;
    }

    char*        data;
    uint32_t    size;
} SC_Data;


#endif /* DATA_ENTRY_H_ */
