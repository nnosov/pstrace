#ifndef __PST_DWARF_OPERATIONS_H__
#define __PST_DWARF_OPERATIONS_H__

/*
 * dwarf_operations.cpp
 *
 *  Created on: Jan 11, 2020
 *      Author: nnosov
 */


#include <elfutils/libdwfl.h>

#include "dwarf/dwarf_stack.h"
#include "utils/list_head.h"
#include "common.h"
#include "context.h"
#include "dwarf_handler.h"


typedef struct __dwarf_op_map dwarf_op_map;

typedef bool (*dwarf_operation)(pst_dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2);

typedef struct __dwarf_op_map {
	int				op_num;		// DWARF Operation DW_OP_XXX
	const char* 	op_name;	// string representation of an operation
	dwarf_operation operation;	// function which handles operation
} dwarf_op_map;

const dwarf_op_map* find_op_map(int op);

#endif /* __PST_DWARF_OPERATIONS_H__ */
