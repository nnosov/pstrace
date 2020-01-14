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

#include "common.h"

char* GetExecutableName(char* path, uint32_t size);

typedef struct __pst_function pst_function;

typedef struct __pst_type : public SC_ListNode {
    __pst_type(const char* n, uint32_t t) : type(t) {
        if(n) {
            name = n;
        }
    }
    std::string name;   // type name
    uint32_t    type;   // DW_AT_XXX type
} pst_type;

typedef struct __pst_parameter : public SC_ListNode {
	__pst_parameter(pst_context* c) : ctx(c)
	{
		die = NULL;
		size = 0;
		type = 0;
		loc = NULL;
		is_return = false;
		is_variable = false;
		has_value = false;
		value = 0;
		line = 0;
	}

	~__pst_parameter()
	{
	    clear();
	}

	void clear();
	pst_type* add_type(const char* name, int type);
	pst_type* next_type(pst_type* t);

	bool handle_dwarf(Dwarf_Die* d);
	bool handle_type(Dwarf_Attribute* param);
	bool print_dwarf();

	Dwarf_Die*					die; 	// DWARF DIE containing parameter's definition
	std::string					name;	// parameter's name
	uint32_t                    line;   // line of parameter definition
	Dwarf_Word					size;	// size of parameter in bytes
	int							type;	// base type of parameter in DW_TAG_XXX types enumeration
	SC_ListHead	                types;	// list of parameter's definitions i.e. 'typedef', 'uint32_t'
	void*						loc;	// pointer to location of parameter's value
	bool						is_return; // whether this parameter is return value of the function
	bool                        is_variable;// whether this parameter is function variable or argument of function
	bool                        has_value;
	uint64_t					value;	// value of parameter
	pst_context*				ctx;
} pst_parameter;

typedef struct __pst_handler pst_handler;

typedef struct __pst_function : public SC_ListNode {
	__pst_function(pst_context* ctx) : ctx(ctx) {
		pc = 0;
		line = -1;
		die = NULL;
		lowpc = 0;
		highpc = 0;
	}

	~__pst_function() {
	    clear();
	}

    void clear();
    pst_parameter* add_param();
    void del_param(pst_parameter* p);
    pst_parameter* next_param(pst_parameter* p);

	bool unwind(Dwfl* dwfl, Dwfl_Module* module, Dwarf_Addr addr);
	bool handle_dwarf(Dwarf_Die* d);
	bool print_dwarf();

	Dwarf_Addr					lowpc;
	Dwarf_Addr					highpc;
	Dwarf_Die*					die; 	// DWARF DIE containing definition of the function
	std::string					name;	// function's name
	SC_ListHead	                params;	// array of function's parameters
	unw_word_t  				pc;		// pointer to the start of the function
	int							line;	// line in code where function is defined
	std::string					file;	// file name (DWARF Compilation Unit) where is function defined
	pst_context*				ctx;
} pst_function;

typedef struct __pst_handler {
	__pst_handler(ucontext_t* hctx) : ctx(hctx)
	{
		caller = NULL;
		module = NULL;
		dwfl = NULL;
		frame = NULL;
		addr = 0;
		handle = 0;
	}

	~__pst_handler()
	{
		if(handle) {
			dlclose(handle);
		}

		clear();
	}

    void clear();
    pst_function* add_function();
    void del_function(pst_function* f);
    pst_function* next_function(pst_function* f);

	void print_dwarf();
	bool unwind();
	bool get_frame();
	bool get_dwarf_function(pst_function* fun);

	pst_context					ctx;		// context of unwinding
	void*						handle;		// process handle
	Dwarf_Addr 					addr;		// address of currently processed function
	Dwfl* 						dwfl;		// DWARF context
	Dwfl_Module* 				module;		// currently processed CU
	Dwarf_Frame* 				frame;		// currently processed stack frame
	void*						caller;		// pointer to the function which requested to unwind stack
	SC_ListHead	                functions;	// list of functions in stack frame
} pst_handler;

#endif /* SC_SYSUTILS_H_ */
