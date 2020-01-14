#pragma once

#include <string.h>
#include <stdlib.h>
#include <execinfo.h>
#include <libunwind.h>

#include "logger/log.h"
#include "linkedlist.h"

typedef enum {
	DWARF_TYPE_INVALID 		= 0,    // no type
	DWARF_TYPE_SIGNED 		= 1,    // signed type
	DWARF_TYPE_UNSIGNED 	= 2,    // unsigned type
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

	char* 				value;
	uint32_t 			size;   // size in bytes except of 'DWARF_TYPE_PIECE', in such case in bits
	int	                type;   // bitmask of DWARF_TYPE_XXX
} dwarf_value;

typedef struct __dwarf_stack : public SC_ListHead {
	void clear() {
		for(dwarf_value* v = (dwarf_value*)First(); v; v = (dwarf_value*)First()) {
			Remove(v);
			delete v;
		}
	}

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

	Dwarf_Attribute*    attr; // attribute which expression currently processed
} dwarf_stack;

typedef struct __pst_context {
	__pst_context(ucontext_t* hctx) : hcontext(hctx)
	{
		offset = 0;
		buff[0] = 0;
		base_addr = 0;
	}

	bool print(const char* fmt, ...);
	void log(SC_LogSeverity severity, const char*fmt, ...);
	uint32_t print_expr_block(Dwarf_Op *exprs, int len, char* buff, uint32_t buff_size, Dwarf_Attribute* attr = 0);
	bool calc_expression(Dwarf_Op *exprs, int expr_len, Dwarf_Attribute* attr);
	bool get_value(uint64_t& value);

	ucontext_t*					hcontext;	// context of signal handler
	unw_context_t				context;	// context of stack trace
	unw_cursor_t				cursor;		// currently examined frame of context
	dwarf_stack					stack;
	Dwarf_Addr					base_addr;	// base address where process loaded

	char						buff[8192]; // stack trace buffer
	uint32_t					offset;		// offset in the 'buff'
} pst_context;


int32_t decode_sleb128(uint8_t *sleb128);
uint32_t decode_uleb128(uint8_t *uleb128);
bool is_location_form(int form);
