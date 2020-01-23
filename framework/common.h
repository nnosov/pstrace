#pragma once

#include <string.h>
#include <stdlib.h>
#include <execinfo.h>
#include <libunwind.h>

#include "logger/log.h"
#include "linkedlist.h"

typedef struct __pst_context {
	__pst_context(ucontext_t* hctx) : hcontext(hctx)
	{
		offset = 0;
		buff[0] = 0;
		base_addr = 0;
		sp = 0;
		cfa = 0;
		curr_frame = NULL;
		next_frame = NULL;
	}

	bool print(const char* fmt, ...);
	void log(SC_LogSeverity severity, const char*fmt, ...);
	uint32_t print_expr_block(Dwarf_Op *exprs, int len, char* buff, uint32_t buff_size, Dwarf_Attribute* attr = 0);

	ucontext_t*					hcontext;	// context of signal handler
	unw_context_t				context;	// context of stack trace
	unw_cursor_t                cursor;
	unw_cursor_t*				curr_frame;	// currently examined frame of context
	unw_cursor_t*               next_frame; // caller frame
	Dwarf_Addr					base_addr;	// base address where process loaded

	char						buff[8192]; // stack trace buffer
	uint32_t					offset;		// offset in the 'buff'
    Dwarf_Addr                  sp;
    Dwarf_Addr                  cfa;
} pst_context;


int32_t decode_sleb128(uint8_t *sleb128);
uint32_t decode_uleb128(uint8_t *uleb128);
bool is_location_form(int form);
