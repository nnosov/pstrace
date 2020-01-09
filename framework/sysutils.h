/*
 * sysutils.h
 *
 *  Created on: Dec 28, 2019
 *      Author: nnosov
 */

#ifndef SC_SYSUTILS_H_
#define SC_SYSUTILS_H_

//system
#include <string>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>
#include <string.h>
#include <ucontext.h>
#include <libunwind.h>
#include <elfutils/libdwfl.h>
#include <dlfcn.h>

char* GetExecutableName(char* path, uint32_t size);

typedef struct __pst_function pst_function;

typedef struct __pst_parameter {
	__pst_parameter(pst_function* fun) : function(fun)
	{
		die = NULL;
		size = 0;
		type = 0;
		loc = NULL;
		is_return = false;
	}

	bool handle_dwarf(Dwarf_Die* d);
	bool handle_type(Dwarf_Attribute* param, bool is_return = false);

	Dwarf_Die*					die; 	// DWARF DIE containing parameter's definition
	std::string					name;	// parameter's name
	Dwarf_Word					size;	// size of parameter in bytes
	int							type;	// base type of parameter in DW_TAG_XXX types enumeration
	std::vector<std::string>	types;	// list of parameters definitions i.e. 'typedef', 'uint32_t'
	void*						loc;	// pointer to location of parameter's value
	bool						is_return; // whether this parameter is return value of the function
	pst_function*				function;
} pst_parameter;

typedef struct __pst_context pst_context;

typedef struct __pst_function {
	__pst_function(pst_context* ctx) : ctx(ctx)
	{
		pc = 0;
		line = -1;
		die = NULL;
		lowpc = 0;
	}

	bool unwind(Dwfl* dwfl, Dwfl_Module* module, Dwarf_Addr addr);
	bool handle_dwarf(Dwarf_Die* d);

	Dwarf_Addr					lowpc;
	Dwarf_Die*					die; 	// DWARF DIE containing definition of the function
	std::string					name;	// function's name
	std::vector<pst_parameter>	params;	// array of function's parameters
	unw_word_t  				pc;		// pointer to the start of the function
	int							line;	// line in code where function is defined
	std::string					file;	// file name (DWARF Compilation Unit) where is function defined
	pst_context*				ctx;
} pst_function;

typedef struct __pst_context {
	__pst_context(ucontext_t* hctx) : hcontext(hctx)
	{
		caller = NULL;
		module = NULL;
		dwfl = NULL;
		offset = 0;
		buff[0] = 0;
		frame = NULL;
		addr = 0;
		base_addr = 0;
		handle = 0;
	}

	~__pst_context()
	{
		if(handle) {
			dlclose(handle);
		}
	}


	bool print(const char* fmt, ...);
	void dwarf_print();
	uint32_t print_expr_block(Dwarf_Op *exprs, int len, char* buff, uint32_t buff_size, Dwarf_Attribute* attr = 0);
	void log(SC_LogSeverity severity, const char*fmt, ...);
	bool unwind();
	bool get_frame();
	bool get_dwarf_function(pst_function& fun);

	ucontext_t*					hcontext;// context of signal handler
	unw_context_t				context;// context of stack trace
	unw_cursor_t				cursor;	// currently examined frame of context
	void*						handle;	// process handle
	Dwarf_Addr 					addr;	// address of currently processed function
	Dwarf_Addr					base_addr;// base address where process loaded
	Dwfl* 						dwfl;	// DWARF context
	Dwfl_Module* 				module;	// currently processed CU
	Dwarf_Frame* 				frame;	// currently processed stack frame
	void*						caller;	// pointer to the function which requested to unwind stack
	std::vector<pst_function>	functions;// array of functions in stack frame
	char						buff[8192]; // stack trace buffer
	uint32_t					offset;	// offset in the 'buff'
} pst_context;

// libdw-based trace call stack implementation
bool LibdwTraceCallStack(pst_context& ctx);

#endif /* SC_SYSUTILS_H_ */
