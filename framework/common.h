#pragma once

#include <string.h>
#include <stdlib.h>
#include <execinfo.h>
#include <libunwind.h>

#include "logger/log.h"
#include "linkedlist.h"

typedef struct __dwarf_value : public SC_ListNode {
	__dwarf_value(uint32_t s)
	{
		size = s;
		value = (char*)malloc(s);
	}

	__dwarf_value(char*v, uint32_t s)
	{
		size = s;
		value = (char*)malloc(s);
		memcpy(value, v, s);
	}

	__dwarf_value()
	{
		value = NULL;
		size = 0;
	}

	~__dwarf_value()
	{
		if(value) {
			free(value);
			value = NULL;
			size = 0;
		}
	}

	char* 		value;
	uint32_t 	size;
} dwarf_value;

typedef struct __dwarf_stack : public SC_ListHead {
	void clear() {
		for(dwarf_value* v = (dwarf_value*)First(); v; v = (dwarf_value*)First()) {
			Remove(v);
			delete v;
		}
	}

	void push(char* v, uint32_t s) {
		dwarf_value* value = new dwarf_value(v, s);
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
