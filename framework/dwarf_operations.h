#pragma once

#include <elfutils/libdwfl.h>

#include "common.h"
#include "linkedlist.h"
#include "sysutils.h"

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
} dwarf_value_type;

typedef struct __dwarf_value : public SC_ListNode {
    __dwarf_value(char*v, uint32_t s, int t)
    {
        size = s;
        value = (char*)malloc(s);
        memcpy(value, v, s);
        type = t;
    }

    ~__dwarf_value()
    {
        if(value) {
            free(value);
            value = NULL;
            size = 0;
        }
    }

    void replace(void* v, uint32_t s, int t)
    {
        if(value) {
            free(value);
            size = 0;
            type = DWARF_TYPE_INVALID;
        }

        value = (char*)malloc(s);
        memcpy(value, v, s);
        size = s;
        type = t;
    }

    bool get_uint(uint64_t& v);
    bool get_int(int64_t& v);
    bool get_generic(uint64_t& v);

    char*               value;
    uint32_t            size;   // size in bytes except of 'DWARF_TYPE_PIECE', in such case in bits
    int                 type;   // bitmask of DWARF_TYPE_XXX
} dwarf_value;

typedef struct __dwarf_stack : public SC_ListHead {
    __dwarf_stack(pst_context* c) : ctx(c) {
        attr = NULL;
    }

    ~__dwarf_stack() {
        for(pst_dwarf_op* op = (pst_dwarf_op*)expr.First(); op; op = (pst_dwarf_op*)expr.First()) {
            expr.Remove(op);
            delete op;
        }
    }

    void clear() {
        for(dwarf_value* v = (dwarf_value*)First(); v; v = (dwarf_value*)First()) {
            Remove(v);
            delete v;
        }
    }

    bool is_expr_equal(__dwarf_stack* rhs);

    void push(void* v, uint32_t s, int t) {
        dwarf_value* value = new dwarf_value((char*)v, s, t);
        InsertFirst(value);
    }

    void push(dwarf_value* value) {
        InsertFirst(value);
    }

    dwarf_value* pop() {
        dwarf_value* value = (dwarf_value*)First();
        if(value) {
            Remove(value);
        }

        return value;
    }

    dwarf_value* get(uint32_t idx = 0) {
        dwarf_value* value = NULL;
        for(value = (dwarf_value*)First(); value && idx; value = (dwarf_value*)Next(value)) {
            idx--;
        }

        return value;
    }

    bool calc_expression(Dwarf_Op *exprs, int expr_len, Dwarf_Attribute* attr, pst_function* fun = NULL);
    bool get_value(uint64_t& value);

    Dwarf_Attribute*    attr;   // attribute which expression currently processed
    SC_ListHead         expr;   // DWARF expression
    pst_context*        ctx;
} dwarf_stack;

typedef struct __dwarf_op_map dwarf_op_map;

typedef bool (*dwarf_operation)(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2);

typedef struct __dwarf_op_map {
	int				op_num;		// DWARF Operation DW_OP_XXX
	const char* 	op_name;	// string representation of an operation
	dwarf_operation operation;	// function which handles operation
} dwarf_op_map;

typedef struct __dwarf_reg_map {
	int				regno;		// platform-dependent register number
	const char*		regname;	// register name
	uint32_t		op_num;	    // number of DWARF DW_OP_reg(x)/DW_OP_breg(x) operation corresponds to this register
} dwarf_reg_map;

const dwarf_op_map* find_op_map(int op);
int find_regnum(uint32_t op);
