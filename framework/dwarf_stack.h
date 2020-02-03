/*
 * dwarf_stack.h
 *
 *  Created on: Jan 29, 2020
 *      Author: nnosov
 */

#ifndef FRAMEWORK_DWARF_STACK_H_
#define FRAMEWORK_DWARF_STACK_H_

#include <stdint.h>

#include "common.h"
#include "allocator.h"
#include "context.h"
#include "dwarf_handler.h"

// -----------------------------------------------------------------------------------
// DWARF Stack value
// -----------------------------------------------------------------------------------

// DWARF Stack value types, bitmask
typedef enum {
    DWARF_TYPE_INVALID      = 0,    // no type
    DWARF_TYPE_SIGNED       = 1,    // signed type
    DWARF_TYPE_UNSIGNED     = 2,    // unsigned type
    DWARF_TYPE_CONST        = 4,    // constant signed/unsigned type
    DWARF_TYPE_GENERIC      = 8,    // size of machine address type
    DWARF_TYPE_CHAR         = 16,   // 1 byte size
    DWARF_TYPE_FLOAT        = 32,   // machine-dependent floating point size
    DWARF_TYPE_REGISTER_LOC = 64,   // value located in register specified as 'value'
    DWARF_TYPE_MEMORY_LOC   = 128,  // value located in memory address specified as 'value'
    DWARF_TYPE_PIECE        = 256,  // piece of whole value located in current value
    DWARF_TYPE_SHORT        = 512,  // 2 byte size
    DWARF_TYPE_INT          = 1024, // 4 byte size
    DWARF_TYPE_LONG         = 2048  // 8 byte size
} pst_dwarf_value_type;

// DWARF Stack value storage
typedef union {
    uint64_t    uint64_v;
#define uint64  uint64_v
    int64_t     int64_v;
#define int64   int64_v
    uint32_t    uint32_v[2];
#define uint32  uint32_v[0]
    int32_t     int32_v[2];
#define int32   int32_v[0]
    uint16_t    uint16_v[4];
#define uint16  uint16_v[0]
    int16_t     int16_v[4];
#define int16   int16_v[0]
    uint8_t     uint8_v[8];
#define uint8   uint8_v[0]
    int8_t      int8_v[8];
#define int8    int8_v[0]
    void*       ptr;
} pst_sized_value;

typedef struct __pst_dwarf_value {
    list_node            node;       // uplink, !!! must be 1st field in structure !!!

    // fields
    pst_sized_value     value;      // value itself
    int                 type;       // value type. bitmask of DWARF_TYPE_XXX
    bool                allocated;
} pst_dwarf_value;

void pst_dwarf_value_init(pst_dwarf_value* value, char* v, uint32_t s, int t);
pst_dwarf_value* pst_dwarf_value_new(char* v, uint32_t s, int t);
void pst_dwarf_value_fini(pst_dwarf_value* value);
void pst_dwarf_value_set(pst_dwarf_value* value, void* v, uint32_t s, int t);

// -----------------------------------------------------------------------------------
// DWARF stack
// -----------------------------------------------------------------------------------

typedef struct __pst_dwarf_stack {
    list_head                   expr;       // DWARF expression
    list_head                   values;     // list of values on the stack
    pst_context*                ctx;        // context of execution
    bool                        allocated;  // whether this object was allocated or not
} pst_dwarf_stack;

void pst_dwarf_stack_init(pst_dwarf_stack* st, pst_context* ctx);
pst_dwarf_stack* pst_dwarf_stack_new(pst_context* ctx);
void pst_dwarf_stack_fini(pst_dwarf_stack* st);

bool pst_dwarf_stack_calc(pst_dwarf_stack* st, Dwarf_Op *exprs, int expr_len, Dwarf_Attribute* attr, pst_function* fun = NULL);
void pst_dwarf_stack_clear(pst_dwarf_stack* st);
bool pst_dwarf_stack_get_value(pst_dwarf_stack* st, uint64_t* value);
pst_dwarf_value* pst_dwarf_stack_get(pst_dwarf_stack* st, uint32_t idx = 0);
pst_dwarf_value* pst_dwarf_stack_pop(pst_dwarf_stack* st);
void pst_dwarf_stack_push_value(pst_dwarf_stack* st, pst_dwarf_value* value);
void pst_dwarf_stack_push(pst_dwarf_stack* st, void* v, uint32_t s, int t);

#endif /* FRAMEWORK_DWARF_STACK_H_ */
