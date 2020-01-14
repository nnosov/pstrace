#pragma once

#include <elfutils/libdwfl.h>

#include "common.h"

typedef struct __dwarf_op_map dwarf_op_map;

typedef bool (*dwarf_operation)(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2);

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
