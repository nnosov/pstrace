/*
 * dwarf_operations.cpp
 *
 *  Created on: Jan 11, 2020
 *      Author: nnosov
 */

#include <dwarf.h>
#include "dwarf_operations.h"
#include "common.h"

bool dw_op_notimpl(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	ctx->log(SEVERITY_ERROR, "0x%lX => %s(0x%lX, 0x%lX) operation is not implemented", map->op_num, map->op_name, op1, op2);
	return false;
}

bool dw_op_addr(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The DW_OP_addr operation has a single operand that encodes a machine
	// address and whose size is the size of an address on the target machine.
	ctx->stack.push(&op1, sizeof(op1), DWARF_TYPE_ADDRESS);
	return true;
}

bool dw_op_deref(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The DW_ OP_deref operation pops the top stack entry and treats it as an address.
	// The popped value must have an integral type. The value retrieved from that address is pushed, and has the generic type.
	// The size of the data retrieved from the dereferenced address is the size of an address on the target machine.
	dwarf_value* value = ctx->stack.pop();
	if(value) {
		uint64_t v = *((uint64_t*)value->value);
		ctx->stack.push(&v, sizeof(v), DWARF_TYPE_GENERIC);

		return true;
	}

	return false;
}

bool dw_op_const_x_u(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// DW_OP_const1u, DW_OP_const2u, DW_OP_const4u, DW_OP_const8u. The single operand of a DW_OP_const<n>u operation provides a 1, 2, 4, or  8-byte unsigned integer constant, respectively.
	uint8_t size = 0;
	switch (map->op_num) {
		case DW_OP_const1u:
			size = 1;
			break;
		case DW_OP_const2u:
			size = 2;
			break;
		case DW_OP_const4u:
			size = 4;
			break;
		case DW_OP_const8u:
			size = 1;
			break;
		default:
			return false;
	}

	ctx->stack.push(&op1, size, DWARF_TYPE_UNSIGNED);

	return true;
}

bool dw_op_const_x_s(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// DW_OP_const1s, DW_OP_const2s, DW_OP_const4s, DW_OP_const8s. The single operand of a DW_OP_const<n>s operation provides a 1, 2, 4, or 8-byte signed integer constant, respectively.
	uint8_t size = 0;
	switch (map->op_num) {
		case DW_OP_const1u:
			size = 1;
			break;
		case DW_OP_const2u:
			size = 2;
			break;
		case DW_OP_const4u:
			size = 4;
			break;
		case DW_OP_const8u:
			size = 1;
			break;
		default:
			return false;
	}

	ctx->stack.push(&op1, size, DWARF_TYPE_SIGNED);

	return true;
}

bool dw_op_constu(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The single operand of the DW_OP_constu operation provides an unsigned LEB128 integer constant.
	uint64_t value = decode_uleb128((unsigned char*)&op1);
	ctx->stack.push(&value, sizeof(value), DWARF_TYPE_UNSIGNED);
	return true;
}

bool dw_op_consts(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The single operand of the DW_OP_consts operation provides a signed LEB128 integer constant.
	uint64_t value = decode_sleb128((unsigned char*)&op1);
	ctx->stack.push(&value, sizeof(value), DWARF_TYPE_SIGNED);
	return true;
}

bool dw_op_dup(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The DW_OP_dup operation duplicates the value (including its type identifier) at the top of the stack.
	dwarf_value* value = ctx->stack.get();
	ctx->stack.push(value->value, value->size, value->type);

	return true;
}

bool dw_op_drop(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The DW_OP_drop operation pops the value (including its type identifier) at the top of the stack.
	dwarf_value* value = ctx->stack.pop();
	free(value);

	return true;
}

bool dw_op_over(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The DW_OP_over operation duplicates the entry currently second in the stack at the top of the stack.
	// This is equivalent to a DW_OP_pick operation, with index 1.

	dwarf_value* value = ctx->stack.get(1);
	ctx->stack.push(value->value, value->size, value->type);

	return true;
}

bool dw_op_pick(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The single operand of the DW_OP_pick operation provides a 1-byte index.
	// A copy of the stack entry (including its type identifier) with the specified index (0 through 255, inclusive) is pushed onto the stack.

	dwarf_value* value = ctx->stack.get(op1);
	if(value) {
		ctx->stack.push(value->value, value->size, value->type);
		return true;
	}

	return false;
}

bool dw_op_swap(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The DW_OP_swap operation swaps the top two stack entries. The entry at the top of the stack (including its type identifier) becomes the second stack
	// entry, and the second entry (including its type identifier) becomes the top of the stack.

	dwarf_value* value1 = ctx->stack.pop();
	dwarf_value* value2 = ctx->stack.pop();
	if(value1 && value2) {
		ctx->stack.push(value1);
		ctx->stack.push(value2);
		return true;
	}

	return false;
}

bool dw_op_rot(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The DW_OP_rot operation rotates the first three stack entries.
	// The entry at the top of the stack (including its type identifier) becomes the third stack entry,
	// the second entry (including its type identifier) becomes the top of the stack,
	// and the third entry (including its type identifier) becomes the second entry

	dwarf_value* value1 = ctx->stack.pop();
	dwarf_value* value2 = ctx->stack.pop();
	dwarf_value* value3 = ctx->stack.pop();
	if(value1 && value2 && value3) {
		ctx->stack.push(value1);
		ctx->stack.push(value3);
		ctx->stack.push(value2);
		return true;
	}

	return false;
}

bool dw_op_abs(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The DW_OP_abs operation pops the top stack entry, interprets it as a signed value and pushes its absolute value.
	// If the absolute value cannot be represented, the result is undefined.

	dwarf_value* value = ctx->stack.get();
	if(value) {
		uint64_t v;
		if(!value->get_int(v)) {
			ctx->log(SEVERITY_ERROR, "Wrong %d size of stack value", value->size);
			return false;
		}

		value->replace(&v, value->size, DWARF_TYPE_SIGNED);
	}

	return false;
}

bool dw_op_and(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The DW_OP_and operation pops the top two stack values, performs a bitwise and operation on the two, and pushes the result.

	dwarf_value* value1 = ctx->stack.pop();
	dwarf_value* value2 = ctx->stack.pop();
	if(value1 && value2) {
		if(value1->type != value2->type) {
			ctx->log(SEVERITY_ERROR, "Different types of two stack values for operation: %s(%d, %d)", map->op_name, value1->type, value2->type);
			return false;
		}
		uint64_t v1 = 0, v2 = 0;
		if(!value1->get_uint(v1)) {
			ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		if(!value2->get_uint(v2)) {
			ctx->log(SEVERITY_ERROR, "Wrong size of 2nd stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}
		uint64_t res = v1 & v2;
		ctx->stack.push(&res, value1->size, value1->type);
		return true;
	}

	return false;
}

bool dw_op_div(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The DW_OP_div operation pops the top two stack values, divides the former second entry by the former top of the stack using signed division, and pushes the result.

	dwarf_value* value1 = ctx->stack.pop();
	dwarf_value* value2 = ctx->stack.pop();
	if(value1 && value2) {
		if(value1->type != value2->type) {
			ctx->log(SEVERITY_ERROR, "Different types of two stack values for operation: %s(%d, %d)", map->op_name, value1->type, value2->type);
			return false;
		}
		int64_t v1 = 0, v2 = 0;
		if(!value1->get_int(v1)) {
			ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		if(!value2->get_int(v2)) {
			ctx->log(SEVERITY_ERROR, "Wrong size of 2nd stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		int64_t res = v2 / v1;
		ctx->stack.push(&res, value1->size, value1->type);
		return true;
	}

	return false;
}

bool dw_op_minus(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The DW_OP_minus operation pops the top two stack values, subtracts the former top of the stack from the former second entry, and pushes the result.

	dwarf_value* value1 = ctx->stack.pop();
	dwarf_value* value2 = ctx->stack.pop();
	if(value1 && value2) {
		if(value1->type != value2->type) {
			ctx->log(SEVERITY_ERROR, "Different types of two stack values for operation: %s(%d, %d)", map->op_name, value1->type, value2->type);
			return false;
		}
		int64_t v1 = 0, v2 = 0;
		if(!value1->get_int(v1)) {
			ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		if(!value2->get_int(v2)) {
			ctx->log(SEVERITY_ERROR, "Wrong size of 2nd stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		int64_t res = v2 - v1;
		ctx->stack.push(&res, value1->size, value1->type);
		return true;
	}

	return false;
}

bool dw_op_mod(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The DW_OP_mod operation pops the top two stack values and pushes the result of the calculation: former second stack entry modulo the former top of the stack.

	dwarf_value* value1 = ctx->stack.pop();
	dwarf_value* value2 = ctx->stack.pop();
	if(value1 && value2) {
		if(value1->type != value2->type) {
			ctx->log(SEVERITY_ERROR, "Different types of two stack values for operation: %s(%d, %d)", map->op_name, value1->type, value2->type);
			return false;
		}
		int64_t v1 = 0, v2 = 0;
		if(!value1->get_int(v1)) {
			ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		if(!value2->get_int(v2)) {
			ctx->log(SEVERITY_ERROR, "Wrong size of 2nd stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		int64_t res = v2 % v1;
		ctx->stack.push(&res, value1->size, value1->type);
		return true;
	}

	return false;
}

bool dw_op_mul(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The DW_OP_mul operation pops the top two stack entries, multiplies them together, and pushes the result.

	dwarf_value* value1 = ctx->stack.pop();
	dwarf_value* value2 = ctx->stack.pop();
	if(value1 && value2) {
		if(value1->type != value2->type) {
			ctx->log(SEVERITY_ERROR, "Different types of two stack values for operation: %s(%d, %d)", map->op_name, value1->type, value2->type);
			return false;
		}
		int64_t v1 = 0, v2 = 0;
		if(!value1->get_int(v1)) {
			ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		if(!value2->get_int(v2)) {
			ctx->log(SEVERITY_ERROR, "Wrong size of 2nd stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		int64_t res = v2 * v1;
		ctx->stack.push(&res, value1->size, value1->type);
		return true;
	}

	return false;
}

bool dw_op_neg(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The DW_OP_neg operation pops the top stack entry, interprets it as a signed value and pushes its negation.
	// If the negation cannot be represented, the result is undefined.

	dwarf_value* value = ctx->stack.get();
	if(value) {
		int64_t v = 0;
		if(!value->get_int(v)) {
			ctx->log(SEVERITY_ERROR, "Wrong size of stack value for operation %s(%d)", map->op_name, value->size);
			return false;
		}
		v *= -1;

		switch (value->size) {
			case 1: {
				int8_t vv = (int8_t)v;
				value->replace(&vv, sizeof(vv), value->type);
				break;
			}
			case 2: {
				int16_t vv = (int16_t)v;
				value->replace(&vv, sizeof(vv), value->type);
				break;
			}
			case 4: {
				int32_t vv = (int32_t)v;
				value->replace(&vv, sizeof(vv), value->type);
				break;
			}
			case 8: {
				value->replace(&v, sizeof(v), value->type);
				break;
			}
			default:
				ctx->log(SEVERITY_ERROR, "Wrong size of stack value for operation %s(%d)", map->op_name, value->size);
				return false;
				break;
		}

		return true;
	}

	return false;
}

bool dw_op_not(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The DW_OP_neg operation pops the top stack entry, interprets it as a signed value and pushes its negation.
	// If the negation cannot be represented, the result is undefined.

	dwarf_value* value = ctx->stack.get();
	if(value) {
		uint64_t v = 0;
		if(!value->get_uint(v)) {
			ctx->log(SEVERITY_ERROR, "Wrong size of stack value for operation %s(%d)", map->op_name, value->size);
			return false;
		}
		v = !v;

		value->replace(&v, value->size, value->type);

		return true;
	}

	return false;
}

bool dw_op_or(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The DW_OP_or operation pops the top two stack entries, performs a bitwise or operation on the two, and pushes the result.

	dwarf_value* value1 = ctx->stack.pop();
	dwarf_value* value2 = ctx->stack.pop();
	if(value1 && value2) {
		if(value1->type != value2->type) {
			ctx->log(SEVERITY_ERROR, "Different types of two stack values for operation: %s(%d, %d)", map->op_name, value1->type, value2->type);
			return false;
		}
		uint64_t v1 = 0, v2 = 0;
		if(!value1->get_uint(v1)) {
			ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		if(!value2->get_uint(v2)) {
			ctx->log(SEVERITY_ERROR, "Wrong size of 2nd stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		uint64_t res = v2 | v1;
		ctx->stack.push(&res, value1->size, value1->type);
		return true;
	}

	return false;
}

bool dw_op_plus(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The DW_OP_plus operation pops the top two stack entries, adds them together, and pushes the result

	dwarf_value* value1 = ctx->stack.pop();
	dwarf_value* value2 = ctx->stack.pop();
	if(value1 && value2) {
		if(value1->type != value2->type) {
			ctx->log(SEVERITY_ERROR, "Different types of two stack values for operation: %s(%d, %d)", map->op_name, value1->type, value2->type);
			return false;
		}
		int64_t v1 = 0, v2 = 0;
		if(!value1->get_int(v1)) {
			ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		if(!value2->get_int(v2)) {
			ctx->log(SEVERITY_ERROR, "Wrong size of 2nd stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		uint64_t res = v2 + v1;
		ctx->stack.push(&res, value1->size, value1->type);
		return true;
	}

	return false;
}

bool dw_op_plus_uconst(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The DW_OP_plus_uconst operation pops the top stack entry, adds it to the unsigned LEB128 constant operand interpreted as the same type as the
	// operand popped from the top of the stack and pushes the result.
	// This operation is supplied specifically to be able to encode more field offsets in two
	// bytes than can be done with “DW_OP_lit<n> DW_OP_plus.”

	dwarf_value* value = ctx->stack.get();
	if(value) {
		if(value->type != DWARF_TYPE_SIGNED && value->type != DWARF_TYPE_UNSIGNED) {
			ctx->log(SEVERITY_ERROR, "Invalid type for operation %s(%d)", map->op_name, value->type);
			return false;
		}

		uint64_t op = decode_uleb128((unsigned char*)&op1);
		if(value->type == DWARF_TYPE_SIGNED) {
			int64_t v = 0;
			if(!value->get_int(v)) {
				ctx->log(SEVERITY_ERROR, "Wrong size of stack value for operation %s(%d)", map->op_name, value->size);
				return false;
			}
			v += op;
			value->replace(&v, sizeof(v), value->type);
		} else {
			uint64_t v = 0;
			if(!value->get_uint(v)) {
				ctx->log(SEVERITY_ERROR, "Wrong size of stack value for operation %s(%d)", map->op_name, value->size);
				return false;
			}
			v += op;
			value->replace(&v, sizeof(v), value->type);
		}

		return true;
	}

	return false;
}

dwarf_op_map dw_op[] = {
		{DW_OP_addr, -1,   0, "DW_OP_addr", dw_op_addr},
		{DW_OP_deref, -1,  0, "DW_OP_deref", dw_op_deref},
		// Constant operations
		{DW_OP_const1u, -1, 0, "DW_OP_const1u", dw_op_const_x_u},
		{DW_OP_const1s, -1, 0, "DW_OP_const1s", dw_op_const_x_s},
		{DW_OP_const2u, -1, 0, "DW_OP_const2u", dw_op_const_x_u},
		{DW_OP_const2s, -1, 0, "DW_OP_const2s", dw_op_const_x_s},
		{DW_OP_const4u, -1, 0, "DW_OP_const4u", dw_op_const_x_u},
		{DW_OP_const4s, -1, 0, "DW_OP_const4s", dw_op_const_x_s},
		{DW_OP_const8u, -1, 0, "DW_OP_const8u", dw_op_const_x_u},
		{DW_OP_const8s, -1, 0, "DW_OP_const8s", dw_op_const_x_s},
		{DW_OP_constu,  -1, 0, "DW_OP_constu",  dw_op_constu},
		{DW_OP_consts,  -1, 0, "DW_OP_consts",  dw_op_consts},
		// DWARF expression stack operations
		{DW_OP_dup,     -1, 0, "DW_OP_dup",     dw_op_dup},
		{DW_OP_drop,    -1, 0, "DW_OP_drop",    dw_op_drop},
		{DW_OP_over,    -1, 0, "DW_OP_over",    dw_op_over},
		{DW_OP_pick,	-1, 0, "DW_OP_pick",	dw_op_pick},
		{DW_OP_swap,	-1, 0, "DW_OP_swap",	dw_op_swap},
		{DW_OP_rot,		-1, 0, "DW_OP_rot",		dw_op_rot},
		{DW_OP_xderef, 	-1, 0, "DW_OP_xderef",	dw_op_notimpl},
		// Arithmetic and Logical Operations
		{DW_OP_abs,		-1,	0, "DW_OP_abs",		dw_op_abs},
		{DW_OP_and, 	-1, 0, "DW_OP_and",		dw_op_and},
		{DW_OP_div,		-1, 0, "DW_OP_div",		dw_op_div},
		{DW_OP_minus,	-1, 0, "DW_OP_minus",	dw_op_minus},
		{DW_OP_mod,		-1, 0, "DW_OP_mod",		dw_op_mod},
		{DW_OP_mul,		-1, 0, "DW_OP_mul",		dw_op_mul},
		{DW_OP_neg, 	-1, 0, "DW_OP_neg",		dw_op_neg},
		{DW_OP_not,		-1, 0, "DW_OP_not", 	dw_op_not},
		{DW_OP_or,		-1, 0, "DW_OP_or",		dw_op_or},
		{DW_OP_plus,	-1, 0, "DW_OP_plus",	dw_op_plus},
		{DW_OP_plus_uconst, 0x0,  0, "DW_OP_plus_uconst", dw_op_plus_uconst},
		// Register location descriptions. From DWARF 5, section 2.6.1.1.3:
		// Register location descriptions describe an object (or a piece of an object) that resides in a register.
		// A register location description must stand alone as the entire description of an object or a piece of an object.

		// GP Registers
		{0x50, 0x0,  "RAX", "DW_OP_reg0"},
		{0x51, 0x1,  "RDX", "DW_OP_reg1"},
		{0x52, 0x2,  "RCX", "DW_OP_reg2"},
		{0x53, 0x3,  "RBX", "DW_OP_reg3"},
		{0x54, 0x4,  "RSI", "DW_OP_reg4"},
		{0x55, 0x5,  "RDI", "DW_OP_reg5"},
		{0x56, 0x6,  "RBP", "DW_OP_reg6"},
		{0x57, 0x7,  "RSP", "DW_OP_reg7"},
		// Extended GP Registers
		{0x58, 0x8,  "R8",  "DW_OP_reg8"},
		{0x59, 0x9,  "R9",  "DW_OP_reg9"},
		{0x5A, 0xA,  "R10", "DW_OP_reg10"},
		{0x5B, 0xB,  "R11", "DW_OP_reg11"},
		{0x5C, 0xC,  "R12", "DW_OP_reg12"},
		{0x5D, 0xD,  "R13", "DW_OP_reg13"},
		{0x5E, 0xE,  "R14", "DW_OP_reg14"},
		{0x5F, 0xF,  "R15", "DW_OP_reg15"},
		{0x60, 0x10, "RIP", "DW_OP_reg16"}, // Return Address (RA) mapped to RIP
		// SSE Vector Registers
		{0x61, 0x11, "XMM0", "DW_OP_reg17"},
		{0x62, 0x12, "XMM1", "DW_OP_reg18"},
		{0x63, 0x13, "XMM2", "DW_OP_reg19"},
		{0x64, 0x14, "XMM3", "DW_OP_reg20"},
		{0x65, 0x15, "XMM4", "DW_OP_reg21"},
		{0x66, 0x16, "XMM5", "DW_OP_reg22"},
		{0x67, 0x17, "XMM6", "DW_OP_reg23"},
		{0x68, 0x18, "XMM7", "DW_OP_reg24"},
		{0x69, 0x19, "XMM8", "DW_OP_reg25"},
		{0x6a, 0x1a, "XMM9", "DW_OP_reg26"},
		{0x6b, 0x1b, "XMM10", "DW_OP_reg27"},
		{0x6c, 0x1c, "XMM11", "DW_OP_reg28"},
		{0x6d, 0x1d, "XMM12", "DW_OP_reg29"},
		{0x6e, 0x1e, "XMM13", "DW_OP_reg30"},
		{0x6f, 0x1f, "XMM14", "DW_OP_reg31"},

		// Register values. DWARF5, section 2.5.1.2
		// Register values are used to describe an object (or a piece of an object) that is located in memory at an address that is contained in
		// a register (possibly offset by some constant)

		// GP Registers
		{0x70, 0x0,  "RAX", "DW_OP_breg0"},
		{0x71, 0x1,  "RDX", "DW_OP_breg1"},
		{0x72, 0x2,  "RCX", "DW_OP_breg2"},
		{0x73, 0x3,  "RBX", "DW_OP_breg3"},
		{0x74, 0x4,  "RSI", "DW_OP_breg4"},
		{0x75, 0x5,  "RDI", "DW_OP_breg5"},
		{0x76, 0x6,  "RBP", "DW_OP_breg6"},
		{0x77, 0x7,  "RSP", "DW_OP_breg7"},
		// Extended GP Registers
		{0x78, 0x8,  "R8",  "DW_OP_breg8"},
		{0x79, 0x9,  "R9",  "DW_OP_breg9"},
		{0x7A, 0xA,  "R10", "DW_OP_breg10"},
		{0x7B, 0xB,  "R11", "DW_OP_breg11"},
		{0x7C, 0xC,  "R12", "DW_OP_breg12"},
		{0x7D, 0xD,  "R13", "DW_OP_breg13"},
		{0x7E, 0xE,  "R14", "DW_OP_breg14"},
		{0x7F, 0xF,  "R15", "DW_OP_breg15"},
		{0x80, 0x10, "RIP", "DW_OP_breg16"}, // Return Address (RA) mapped to RIP
		// SSE Vector Registers
		{0x81, 0x11, "XMM0", "DW_OP_breg17"},
		{0x82, 0x12, "XMM1", "DW_OP_breg18"},
		{0x83, 0x13, "XMM2", "DW_OP_breg19"},
		{0x84, 0x14, "XMM3", "DW_OP_breg20"},
		{0x85, 0x15, "XMM4", "DW_OP_breg21"},
		{0x86, 0x16, "XMM5", "DW_OP_breg22"},
		{0x87, 0x17, "XMM6", "DW_OP_breg23"},
		{0x88, 0x18, "XMM7", "DW_OP_breg24"},
		{0x89, 0x19, "XMM8", "DW_OP_breg25"},
		{0x8a, 0x1a, "XMM9", "DW_OP_breg26"},
		{0x8b, 0x1b, "XMM10", "DW_OP_breg27"},
		{0x8c, 0x1c, "XMM11", "DW_OP_breg28"},
		{0x8d, 0x1d, "XMM12", "DW_OP_breg29"},
		{0x8e, 0x1e, "XMM13", "DW_OP_breg30"},
		{0x8f, 0x1f, "XMM14", "DW_OP_breg31"},

		// The DW_OP_regx operation has a single unsigned LEB128 literal operand that encodes the name of a register
		{0x90, -1,   0,     "DW_OP_regx"},
		// The DW_OP_fbreg operation provides a signed LEB128 offset from the address specified by the location description in the DW_AT_frame_base
		// attribute of the current function. This is typically a stack pointer register plus or minus some offset
		{0x91, -1,   0,     "DW_OP_fbreg"},
		// The DW_OP_bregx operation provides the sum of two values specified by its two operands.
		// The first operand is a register number which is specified by an unsigned LEB128 number. The second operand is a signed LEB128 offset.
		{0x92, -1,   0,     "DW_OP_bregx"},
		{0x9C, -1,   0,     "DW_OP_call_frame_cfa"},
		// DWARF5, Section 2.6.1.1.4:
		// The DW_OP_stack_value operation specifies that the object does not exist in memory but its value is nonetheless known and is at the top of the DWARF
		// expression stack. In this form of location description, the DWARF expression represents the actual value of the object, rather than its location.
		// The DW_OP_stack_value operation terminates the expression.
		{0x9F, -1,   0,     "DW_OP_stack_value"},
		// This opcode has two operands, the first one is uleb128 length and the second is block of that length, containing either a
		// simple register or DWARF expression
		{0xF3, -1,   0,     "DW_OP_GNU_entry_value"},
};

const dwarf_op_map* find_op_map(int op)
{
	for(uint32_t i = 0; i < sizeof(dw_op) / sizeof(dwarf_op_map); ++i) {
		if(dw_op[i].op_num == op) {
			return &dw_op[i];
		}
	}

	return NULL;
}
