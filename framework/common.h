#pragma once

#include <string.h>
#include <stdlib.h>
#include <execinfo.h>
#include <libunwind.h>

#include "logger/log.h"
#include "linkedlist.h"

#define USE_LIBUNWIND

typedef struct __pst_context {
	__pst_context(ucontext_t* hctx) : hcontext(hctx)
	{
	    clean_print();
		base_addr = 0;
		sp = 0;
		cfa = 0;
		curr_frame = NULL;
		next_frame = NULL;
		frame = NULL;
		dwfl = NULL;
		module = NULL;
	}

	bool print(const char* fmt, ...);
	void log(SC_LogSeverity severity, const char*fmt, ...);
	uint32_t print_expr_block(Dwarf_Op *exprs, int len, char* buff, uint32_t buff_size, Dwarf_Attribute* attr = 0);
	void print_registers(int from, int to);
	void print_stack(int max, uint64_t next_cfa);
	void clean_print() {
	    buff[0] = 0;
	    offset = 0;
	}

	const char* get_print() {
	    return buff;
	}

	ucontext_t*					hcontext;	// context of signal handler
	unw_context_t				context;	// context of stack trace
	unw_cursor_t                cursor;
	unw_cursor_t*				curr_frame;	// callee libunwind frame
	unw_cursor_t*               next_frame; // caller libunwind frame
	Dwarf_Addr					base_addr;	// base address where process loaded

    Dwarf_Addr                  sp;
    Dwarf_Addr                  cfa;
    Dwarf_Frame*                frame;      // currently examined libdwfl frame
    Dwfl*                       dwfl;       // DWARF context
    Dwfl_Module*                module;     // currently processed CU

protected:
	char						buff[8192]; // stack trace buffer
	uint32_t					offset;		// offset in the 'buff'
} pst_context;


int32_t decode_sleb128(uint8_t *sleb128);
uint32_t decode_uleb128(uint8_t *uleb128);
bool is_location_form(int form);
