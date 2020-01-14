/*
 * dwarf_operations.cpp
 *
 *  Created on: Jan 11, 2020
 *      Author: nnosov
 */

#include <dwarf.h>
#include <inttypes.h>

#include "dwarf_operations.h"
#include "common.h"

dwarf_reg_map	reg_map[] = {
	// GP Registers
	{0x0,  "RAX",   DW_OP_reg0},
	{0x1,  "RDX",   DW_OP_reg1},
	{0x2,  "RCX",   DW_OP_reg2},
	{0x3,  "RBX",   DW_OP_reg3},
	{0x4,  "RSI",   DW_OP_reg4},
	{0x5,  "RDI",   DW_OP_reg5},
	{0x6,  "RBP",   DW_OP_reg6},
	{0x7,  "RSP",   DW_OP_reg7},
	// Extended GP Registers
	{0x8,  "R8",    DW_OP_reg8},
	{0x9,  "R9",    DW_OP_reg9},
	{0xA,  "R10",   DW_OP_reg10},
	{0xB,  "R11",   DW_OP_reg11},
	{0xC,  "R12",   DW_OP_reg12},
	{0xD,  "R13",   DW_OP_reg13},
	{0xE,  "R14",   DW_OP_reg14},
	{0xF,  "R15",   DW_OP_reg15},
	{0x10, "RIP",   DW_OP_reg16}, // Return Address (RA) mapped to RIP
	// SSE Vector Registers
	{0x11, "XMM0",  DW_OP_reg17},
	{0x12, "XMM1",  DW_OP_reg18},
	{0x13, "XMM2",  DW_OP_reg19},
	{0x14, "XMM3",  DW_OP_reg20},
	{0x15, "XMM4",  DW_OP_reg21},
	{0x16, "XMM5",  DW_OP_reg22},
	{0x17, "XMM6",  DW_OP_reg23},
	{0x18, "XMM7",  DW_OP_reg24},
	{0x19, "XMM8",  DW_OP_reg25},
	{0x1a, "XMM9",  DW_OP_reg26},
	{0x1b, "XMM10", DW_OP_reg27},
	{0x1c, "XMM11", DW_OP_reg28},
	{0x1d, "XMM12", DW_OP_reg29},
	{0x1e, "XMM13", DW_OP_reg30},
	{0x1f, "XMM14", DW_OP_reg31},

    // GP Registers
    {0x0,  "RAX",   DW_OP_breg0},
    {0x1,  "RDX",   DW_OP_breg1},
    {0x2,  "RCX",   DW_OP_breg2},
    {0x3,  "RBX",   DW_OP_breg3},
    {0x4,  "RSI",   DW_OP_breg4},
    {0x5,  "RDI",   DW_OP_breg5},
    {0x6,  "RBP",   DW_OP_breg6},
    {0x7,  "RSP",   DW_OP_breg7},
    // Extended GP Registers
    {0x8,  "R8",    DW_OP_breg8},
    {0x9,  "R9",    DW_OP_breg9},
    {0xA,  "R10",   DW_OP_breg10},
    {0xB,  "R11",   DW_OP_breg11},
    {0xC,  "R12",   DW_OP_breg12},
    {0xD,  "R13",   DW_OP_breg13},
    {0xE,  "R14",   DW_OP_breg14},
    {0xF,  "R15",   DW_OP_breg15},
    {0x10, "RIP",   DW_OP_breg16}, // Return Address (RA) mapped to RIP
    // SSE Vector Registers
    {0x11, "XMM0",  DW_OP_breg17},
    {0x12, "XMM1",  DW_OP_breg18},
    {0x13, "XMM2",  DW_OP_breg19},
    {0x14, "XMM3",  DW_OP_breg20},
    {0x15, "XMM4",  DW_OP_breg21},
    {0x16, "XMM5",  DW_OP_breg22},
    {0x17, "XMM6",  DW_OP_breg23},
    {0x18, "XMM7",  DW_OP_breg24},
    {0x19, "XMM8",  DW_OP_breg25},
    {0x1a, "XMM9",  DW_OP_breg26},
    {0x1b, "XMM10", DW_OP_breg27},
    {0x1c, "XMM11", DW_OP_breg28},
    {0x1d, "XMM12", DW_OP_breg29},
    {0x1e, "XMM13", DW_OP_breg30},
    {0x1f, "XMM14", DW_OP_breg31},
};

// not implemented operations
bool dw_op_notimpl(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
    stack->ctx->log(SEVERITY_ERROR, "%s(0x%lX, 0x%lX) operation is not implemented", map->op_name, op1, op2);
	return false;
}

// The DW_OP_addr operation has a single operand that encodes a machine
// address and whose size is the size of an address on the target machine.
bool dw_op_addr(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
    stack->push(&op1, sizeof(op1), DWARF_TYPE_MEMORY_LOC | DWARF_TYPE_GENERIC);
	return true;
}

// The DW_ OP_deref operation pops the top stack entry and treats it as an address.
// The popped value must have an integral type. The value retrieved from that address is pushed, and has the generic type.
// The size of the data retrieved from the dereferenced address is the size of an address on the target machine.
bool dw_op_deref(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	dwarf_value* value = stack->pop();
	if(value) {
		uint64_t v = *((uint64_t*)value->value);
		stack->push(&v, sizeof(v), DWARF_TYPE_GENERIC);

		return true;
	}

	return false;
}

// DW_OP_const1u, DW_OP_const2u, DW_OP_const4u, DW_OP_const8u. The single operand of a DW_OP_const<n>u operation provides a 1, 2, 4, or  8-byte unsigned integer constant, respectively.
// These operations push a value with the generic type
bool dw_op_const_x_u(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	uint8_t size = 0;
	dwarf_value_type type = DWARF_TYPE_UNSIGNED;
	switch (map->op_num) {
		case DW_OP_const1u:
			size = 1;
			type = DWARF_TYPE_CHAR;
			break;
		case DW_OP_const2u:
			size = 2;
			type = DWARF_TYPE_SHORT;
			break;
		case DW_OP_const4u:
			size = 4;
			type = DWARF_TYPE_INT;
			break;
		case DW_OP_const8u:
			size = 8;
			type = DWARF_TYPE_LONG;
			break;
		default:
			return false;
	}

	stack->push(&op1, size, type | DWARF_TYPE_CONST | DWARF_TYPE_GENERIC);

	return true;
}

// DW_OP_const1s, DW_OP_const2s, DW_OP_const4s, DW_OP_const8s. The single operand of a DW_OP_const<n>s operation provides a 1, 2, 4, or 8-byte signed integer constant, respectively.
// These operations push a value with the generic type
bool dw_op_const_x_s(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	uint8_t size; int64_t v;
	dwarf_value_type type = DWARF_TYPE_SIGNED;
	switch (map->op_num) {
		case DW_OP_const1s:
			v = (int8_t)op1;
			size = sizeof(int8_t);
			type = DWARF_TYPE_CHAR;
			break;
		case DW_OP_const2s:
			v = (int16_t)op1;
			size = sizeof(int16_t);
			type = DWARF_TYPE_SHORT;
			break;
		case DW_OP_const4s:
			v = (int32_t)op1;
			size = sizeof(int32_t);
			type = DWARF_TYPE_INT;
			break;
		case DW_OP_const8s:
			v = (int64_t)op1;
			size = sizeof(int64_t);
			type = DWARF_TYPE_LONG;
			break;
		default:
			return false;
	}

	stack->push(&v, size, type | DWARF_TYPE_CONST | DWARF_TYPE_GENERIC);

	return true;
}

// The single operand of the DW_OP_constu operation provides an unsigned LEB128 integer constant.
bool dw_op_constu(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	uint64_t value = decode_uleb128((unsigned char*)&op1);
	stack->push(&value, sizeof(value), DWARF_TYPE_LONG | DWARF_TYPE_UNSIGNED | DWARF_TYPE_CONST | DWARF_TYPE_GENERIC);
	return true;
}

bool dw_op_consts(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	// The single operand of the DW_OP_consts operation provides a signed LEB128 integer constant.
	int64_t value = decode_sleb128((unsigned char*)&op1);
	stack->push(&value, sizeof(value),  DWARF_TYPE_LONG | DWARF_TYPE_SIGNED | DWARF_TYPE_CONST | DWARF_TYPE_GENERIC);
	return true;
}

// The DW_OP_dup operation duplicates the value (including its type identifier) at the top of the stack.
bool dw_op_dup(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	dwarf_value* value = stack->get();
	stack->push(value->value, value->size, value->type);

	return true;
}

// The DW_OP_drop operation pops the value (including its type identifier) at the top of the stack.
bool dw_op_drop(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	dwarf_value* value = stack->pop();
	free(value);

	return true;
}

// The DW_OP_over operation duplicates the entry currently second in the stack at the top of the stack.
// This is equivalent to a DW_OP_pick operation, with index 1.
bool dw_op_over(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	dwarf_value* value = stack->get(1);
	stack->push(value->value, value->size, value->type);

	return true;
}

// The single operand of the DW_OP_pick operation provides a 1-byte index.
// A copy of the stack entry (including its type identifier) with the specified index (0 through 255, inclusive) is pushed onto the stack.
bool dw_op_pick(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	dwarf_value* value = stack->get(op1);
	if(value) {
	    stack->push(value->value, value->size, value->type);
		return true;
	}

	return false;
}

// The DW_OP_swap operation swaps the top two stack entries. The entry at the top of the stack (including its type identifier) becomes the second stack
// entry, and the second entry (including its type identifier) becomes the top of the stack.
bool dw_op_swap(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	dwarf_value* value1 = stack->pop();
	dwarf_value* value2 = stack->pop();
	if(value1 && value2) {
	    stack->push(value1);
	    stack->push(value2);
		return true;
	}

	if(value1) {
	    delete(value1);
	}
	if(value2) {
	    delete(value2);
	}

	return false;
}

// The DW_OP_rot operation rotates the first three stack entries.
// The entry at the top of the stack (including its type identifier) becomes the third stack entry,
// the second entry (including its type identifier) becomes the top of the stack,
// and the third entry (including its type identifier) becomes the second entry
bool dw_op_rot(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	dwarf_value* value1 = stack->pop();
	dwarf_value* value2 = stack->pop();
	dwarf_value* value3 = stack->pop();
	if(value1 && value2 && value3) {
	    stack->push(value1);
	    stack->push(value3);
	    stack->push(value2);
		return true;
	}

    if(value1) {
        free(value1);
    }
    if(value2) {
        free(value2);
    }
    if(value3) {
        free(value3);
    }

	return false;
}

// The DW_OP_abs operation pops the top stack entry, interprets it as a signed value and pushes its absolute value.
// If the absolute value cannot be represented, the result is undefined.
bool dw_op_abs(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	dwarf_value* value = stack->get();
	if(value) {
		int64_t v;
		if(!value->get_int(v)) {
			stack->ctx->log(SEVERITY_ERROR, "Wrong %d size of stack value", value->size);
			return false;
		}
		uint64_t res = llabs(v);
		value->replace(&res, value->size, DWARF_TYPE_UNSIGNED | DWARF_TYPE_GENERIC | DWARF_TYPE_LONG);
	}

	return false;
}

// The DW_OP_and operation pops the top two stack values, performs a bitwise and operation on the two, and pushes the result.
bool dw_op_and(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	dwarf_value* value1 = stack->get(0);
	dwarf_value* value2 = stack->get(1);
	if(value1 && value2) {
		if(!(value1->type & value2->type)) {
		    stack->ctx->log(SEVERITY_ERROR, "Different types of two stack values for operation: %s(%0x%X, %0x%X)", map->op_name, value1->type, value2->type);
			return false;
		}
		uint64_t v1 = 0, v2 = 0;
		if(!value1->get_generic(v1)) {
		    stack->ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		if(!value2->get_generic(v2)) {
		    stack->ctx->log(SEVERITY_ERROR, "Wrong size of 2nd stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}
		uint64_t res = v1 & v2;
		stack->pop(); stack->pop();
		stack->push(&res, value1->size, DWARF_TYPE_GENERIC);
		return true;
	}

	return false;
}

// The DW_OP_div operation pops the top two stack values, divides the former second entry by the former top of the stack using signed division, and pushes the result.
bool dw_op_div(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	dwarf_value* value1 = stack->get(0);
	dwarf_value* value2 = stack->get(1);
	if(value1 && value2) {
		if(!(value1->type & value2->type)) {
		    stack->ctx->log(SEVERITY_ERROR, "Different types of two stack values for operation: %s(%0x%X, %0x%X)", map->op_name, value1->type, value2->type);
			return false;
		}

		if(value2->type & DWARF_TYPE_SIGNED) {
		    int64_t sig2;
		    if(!value2->get_int(sig2)) {
		        stack->ctx->log(SEVERITY_ERROR, "Wrong size of 2nd stack value for operation %s(%d)", map->op_name, value1->size);
		        return false;
		    }
		    if(value1->type & DWARF_TYPE_SIGNED) {
		        int64_t sig1;
		        if(!value1->get_int(sig1)) {
		            stack->ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
		            return false;
		        }
		        if(sig1 == 0) {
		            stack->ctx->log(SEVERITY_ERROR, "Division by zero for operation %s(0x%lX, 0x%lX)", map->op_name, sig1, sig2);
		            return false;
		        }
		        uint64_t res = sig2 / sig1;
		        stack->pop(); stack->pop();
		        stack->push(&res, sizeof(res), DWARF_TYPE_UNSIGNED | DWARF_TYPE_GENERIC);
		        return true;
		    } else {
                uint64_t unsig1;
                if(!value1->get_uint(unsig1)) {
                    stack->ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
                    return false;
                }
                if(unsig1 == 0) {
                    stack->ctx->log(SEVERITY_ERROR, "Division by zero for operation %s(0x%lX, 0x%lX)", map->op_name, unsig1, sig2);
                    return false;
                }
                int64_t res = sig2 / unsig1;
                stack->pop(); stack->pop();
                stack->push(&res, sizeof(res), DWARF_TYPE_SIGNED | DWARF_TYPE_GENERIC);
                return true;
		    }
		} else {
            uint64_t unsig2;
            if(!value2->get_uint(unsig2)) {
                stack->ctx->log(SEVERITY_ERROR, "Wrong size of 2nd stack value for operation %s(%d)", map->op_name, value1->size);
                return false;
            }
            if(value1->type & DWARF_TYPE_SIGNED) {
                int64_t sig1;
                if(!value1->get_int(sig1)) {
                    stack->ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
                    return false;
                }
                if(sig1 == 0) {
                    stack->ctx->log(SEVERITY_ERROR, "Division by zero for operation %s(0x%lX, 0x%lX)", map->op_name, sig1, sig1);
                    return false;
                }
                int64_t res = unsig2 / sig1;
                stack->pop(); stack->pop();
                stack->push(&res, sizeof(res), DWARF_TYPE_SIGNED | DWARF_TYPE_GENERIC);
                return true;
            } else {
                uint64_t unsig1;
                if(!value1->get_uint(unsig1)) {
                    stack->ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
                    return false;
                }
                if(unsig1 == 0) {
                    stack->ctx->log(SEVERITY_ERROR, "Division by zero for operation %s(0x%lX, 0x%lX)", map->op_name, unsig1, unsig2);
                    return false;
                }
                uint64_t res = unsig2 / unsig1;
                stack->pop(); stack->pop();
                stack->push(&res, sizeof(res), DWARF_TYPE_UNSIGNED | DWARF_TYPE_GENERIC);
                return true;
            }
		}
	}

	return false;
}

// The DW_OP_minus operation pops the top two stack values, subtracts the former top of the stack from the former second entry, and pushes the result.
bool dw_op_minus(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	dwarf_value* value1 = stack->get(0);
	dwarf_value* value2 = stack->get(1);
	if(value1 && value2) {
		if(!(value1->type & value2->type)) {
		    stack->ctx->log(SEVERITY_ERROR, "Different types of two stack values for operation: %s(%d, %d)", map->op_name, value1->type, value2->type);
			return false;
		}
		// use arithmetic by modulo 1 plus
		uint64_t unsig1; uint64_t unsig2;
		if(!value1->get_uint(unsig1)) {
		    stack->ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
		    return false;
		}
        if(!value2->get_uint(unsig2)) {
            stack->ctx->log(SEVERITY_ERROR, "Wrong size of 2nd stack value for operation %s(%d)", map->op_name, value2->size);
            return false;
        }
		int res_type = DWARF_TYPE_GENERIC;
		if(value2->type & DWARF_TYPE_MEMORY_LOC) {
		    res_type |= DWARF_TYPE_MEMORY_LOC;
		}
		uint64_t res = unsig2 - unsig1;
		stack->pop(); stack->pop();
		stack->push(&res, sizeof(res), res_type);

		return true;
	}

	return false;
}

// The DW_OP_mod operation pops the top two stack values and pushes the result of the calculation: former second stack entry modulo the former top of the stack.
bool dw_op_mod(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	dwarf_value* value1 = stack->get(0);
	dwarf_value* value2 = stack->get(1);
	if(value1 && value2) {
		if(!(value1->type & value2->type)) {
		    stack->ctx->log(SEVERITY_ERROR, "Different types of two stack values for operation: %s(%d, %d)", map->op_name, value1->type, value2->type);
			return false;
		}
		uint64_t v1 = 0, v2 = 0;
		if(!value1->get_uint(v1)) {
		    stack->ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		if(v1 == 0) {
		    stack->ctx->log(SEVERITY_ERROR, "Division by zero requested, aborting.");
		    return false;
		}

		if(!value2->get_uint(v2)) {
		    stack->ctx->log(SEVERITY_ERROR, "Wrong size of 2nd stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		uint64_t res = v2 % v1;
		stack->pop(); stack->pop();
		stack->push(&res, value1->size, DWARF_TYPE_UNSIGNED | DWARF_TYPE_GENERIC);
		return true;
	}

	return false;
}

// The DW_OP_mul operation pops the top two stack entries, multiplies them together, and pushes the result.
bool dw_op_mul(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
    dwarf_value* value1 = stack->get(0);
    dwarf_value* value2 = stack->get(1);
    if(value1 && value2) {
        if(!(value1->type & value2->type)) {
            stack->ctx->log(SEVERITY_ERROR, "Different types of two stack values for operation: %s(%0x%X, %0x%X)", map->op_name, value1->type, value2->type);
            return false;
        }

        if(value2->type & DWARF_TYPE_SIGNED) {
            int64_t sig2;
            if(!value2->get_int(sig2)) {
                stack->ctx->log(SEVERITY_ERROR, "Wrong size of 2nd stack value for operation %s(%d)", map->op_name, value1->size);
                return false;
            }
            if(value1->type & DWARF_TYPE_SIGNED) {
                int64_t sig1;
                if(!value1->get_int(sig1)) {
                    stack->ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
                    return false;
                }
                if(sig1 == 0) {
                    stack->ctx->log(SEVERITY_ERROR, "Division by zero for operation %s(0x%lX, 0x%lX)", map->op_name, sig1, sig2);
                    return false;
                }
                uint64_t res = sig2 * sig1;
                stack->pop(); stack->pop();
                stack->push(&res, sizeof(res), DWARF_TYPE_UNSIGNED | DWARF_TYPE_GENERIC);
                return true;
            } else {
                uint64_t unsig1;
                if(!value1->get_uint(unsig1)) {
                    stack->ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
                    return false;
                }
                if(unsig1 == 0) {
                    stack->ctx->log(SEVERITY_ERROR, "Division by zero for operation %s(0x%lX, 0x%lX)", map->op_name, unsig1, sig2);
                    return false;
                }
                int64_t res = sig2 * unsig1;
                stack->pop(); stack->pop();
                stack->push(&res, sizeof(res), DWARF_TYPE_SIGNED | DWARF_TYPE_GENERIC);
                return true;
            }
        } else {
            uint64_t unsig2;
            if(!value2->get_uint(unsig2)) {
                stack->ctx->log(SEVERITY_ERROR, "Wrong size of 2nd stack value for operation %s(%d)", map->op_name, value1->size);
                return false;
            }
            if(value1->type & DWARF_TYPE_SIGNED) {
                int64_t sig1;
                if(!value1->get_int(sig1)) {
                    stack->ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
                    return false;
                }
                if(sig1 == 0) {
                    stack->ctx->log(SEVERITY_ERROR, "Division by zero for operation %s(0x%lX, 0x%lX)", map->op_name, sig1, sig1);
                    return false;
                }
                int64_t res = unsig2 * sig1;
                stack->pop(); stack->pop();
                stack->push(&res, sizeof(res), DWARF_TYPE_SIGNED | DWARF_TYPE_GENERIC);
                return true;
            } else {
                uint64_t unsig1;
                if(!value1->get_uint(unsig1)) {
                    stack->ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
                    return false;
                }
                if(unsig1 == 0) {
                    stack->ctx->log(SEVERITY_ERROR, "Division by zero for operation %s(0x%lX, 0x%lX)", map->op_name, unsig1, unsig2);
                    return false;
                }
                uint64_t res = unsig2 * unsig1;
                stack->pop(); stack->pop();
                stack->push(&res, sizeof(res), DWARF_TYPE_UNSIGNED | DWARF_TYPE_GENERIC);
                return true;
            }
        }
    }

    return false;
}

// The DW_OP_neg operation pops the top stack entry, interprets it as a signed value and pushes its negation.
// If the negation cannot be represented, the result is undefined.
bool dw_op_neg(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	dwarf_value* value = stack->get();
	if(value) {
		int64_t v = 0;
		if(!value->get_int(v)) {
			stack->ctx->log(SEVERITY_ERROR, "Wrong size of stack value for operation %s(%d)", map->op_name, value->size);
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
				stack->ctx->log(SEVERITY_ERROR, "Wrong size of stack value for operation %s(%d)", map->op_name, value->size);
				return false;
				break;
		}

		return true;
	}

	return false;
}

// The DW_OP_not operation pops the top stack entry, and pushes its bitwise complement.
bool dw_op_not(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	dwarf_value* value = stack->get();
	if(value) {
		uint64_t v = 0;
		if(!value->get_uint(v)) {
			stack->ctx->log(SEVERITY_ERROR, "Wrong size of stack value for operation %s(%d)", map->op_name, value->size);
			return false;
		}
		v = ~v;

		value->replace(&v, value->size, value->type);

		return true;
	}

	return false;
}

// The DW_OP_or operation pops the top two stack entries, performs a bitwise or operation on the two, and pushes the result.
bool dw_op_or(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
    dwarf_value* value1 = stack->get(0);
    dwarf_value* value2 = stack->get(1);
	if(value1 && value2) {
		if(!(value1->type & value2->type)) {
			stack->ctx->log(SEVERITY_ERROR, "Different types of two stack values for operation: %s(%d, %d)", map->op_name, value1->type, value2->type);
			return false;
		}
		uint64_t v1 = 0, v2 = 0;
		if(!value1->get_uint(v1)) {
			stack->ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		if(!value2->get_uint(v2)) {
			stack->ctx->log(SEVERITY_ERROR, "Wrong size of 2nd stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		uint64_t res = v2 | v1;
        stack->pop(); stack->pop();
		stack->push(&res, value1->size, value1->type);
		return true;
	}

	return false;
}

// The DW_OP_plus operation pops the top two stack entries, adds them together, and pushes the result
bool dw_op_plus(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	dwarf_value* value1 = stack->get(0);
	dwarf_value* value2 = stack->get(1);
	if(value1 && value2) {
		if(!(value1->type & value2->type)) {
			stack->ctx->log(SEVERITY_ERROR, "Different types of two stack values for operation: %s(%d, %d)", map->op_name, value1->type, value2->type);
			return false;
		}
		int64_t v1 = 0, v2 = 0;
		if(!value1->get_int(v1)) {
			stack->ctx->log(SEVERITY_ERROR, "Wrong size of 1st stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		if(!value2->get_int(v2)) {
			stack->ctx->log(SEVERITY_ERROR, "Wrong size of 2nd stack value for operation %s(%d)", map->op_name, value1->size);
			return false;
		}

		uint64_t res = v2 + v1;
		stack->pop(); stack->pop();
		stack->push(&res, value1->size, value1->type);
		return true;
	}

	return false;
}

// The DW_OP_plus_uconst operation pops the top stack entry, adds it to the unsigned LEB128 constant operand interpreted as the same type as the
// operand popped from the top of the stack and pushes the result.
// This operation is supplied specifically to be able to encode more field offsets in two
// bytes than can be done with “DW_OP_lit<n> DW_OP_plus.”
bool dw_op_plus_uconst(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	dwarf_value* value = stack->get();
	if(value) {
		if(value->type != DWARF_TYPE_SIGNED && value->type != DWARF_TYPE_UNSIGNED) {
			stack->ctx->log(SEVERITY_ERROR, "Invalid type for operation %s(%d)", map->op_name, value->type);
			return false;
		}

		uint64_t op = decode_uleb128((unsigned char*)&op1);
		if(value->type == DWARF_TYPE_SIGNED) {
			int64_t v = 0;
			if(!value->get_int(v)) {
				stack->ctx->log(SEVERITY_ERROR, "Wrong size of stack value for operation %s(%d)", map->op_name, value->size);
				return false;
			}
			v += op;
			value->replace(&v, sizeof(v), value->type);
		} else {
			uint64_t v = 0;
			if(!value->get_uint(v)) {
				stack->ctx->log(SEVERITY_ERROR, "Wrong size of stack value for operation %s(%d)", map->op_name, value->size);
				return false;
			}
			v += op;
			value->replace(&v, sizeof(v), value->type);
		}

		return true;
	}

	return false;
}

// Register location descriptions. Describe an object (or a piece of an object) that resides in a register.
// A register location description must stand alone as the entire description of an object or a piece of an object.
// The DW_OP_regx operation has a single unsigned LEB128 literal operand that encodes the name of a register
bool dw_op_reg_x(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	if(map->op_num != DW_OP_regx && (map->op_num < DW_OP_reg0 || map->op_num > DW_OP_reg31)) {
		return false;
	}

	uint64_t regno = 0;
	if(map->op_num == DW_OP_regx) {
		regno = decode_uleb128((unsigned char*)&op1);
	} else {
		regno = map->op_num - DW_OP_reg0;
	}

	stack->push(&regno, sizeof(regno), DWARF_TYPE_REGISTER_LOC);

	return true;
}

// DWARF5, section 2.5.1.2  Register values are used to describe an object (or a piece of an object) that is located in memory at an address that is contained in a register (possibly offset by some constant)
// The DW_OP_bregx operation provides the sum of two values specified by its two operands.
// The first operand is a register number which is specified by an unsigned LEB128 number. The second operand is a signed LEB128 offset.
bool dw_op_breg_x(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	if(map->op_num != DW_OP_bregx && (map->op_num < DW_OP_breg0 || map->op_num > DW_OP_breg31)) {
		return false;
	}

	int regno = -1; int64_t off = 0;
	if(map->op_num == DW_OP_bregx) {
		regno = decode_uleb128((unsigned char*)&op1);
		off = decode_sleb128((unsigned char*)&op2);
	} else {
		regno = find_regnum(map->op_num);
		off = decode_sleb128((unsigned char*)&op1);
	}

	unw_word_t val = 0;
	if(unw_get_reg(&stack->ctx->cursor, regno, &val)) {
		return false;
	}

	val += off;
	stack->push(&val, sizeof(val), DWARF_TYPE_MEMORY_LOC | DWARF_TYPE_GENERIC);

	return true;
}

// The DW_OP_lit<n> operations encode the unsigned literal values from 0 through 31, inclusive.
// Operations other than DW_OP_const_type push a value with the generic type.
bool dw_op_lit_x(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	if(map->op_num < DW_OP_lit0 || map->op_num > DW_OP_lit31) {
		return false;
	}

	uint64_t val = map->op_num - DW_OP_lit0;
	stack->push(&val, sizeof(val), DWARF_TYPE_GENERIC);

	return true;
}

// The DW_OP_stack_value operation specifies that the object does not exist in memory but its value is nonetheless known and is at the top of the DWARF
// expression stack. In this form of location description, the DWARF expression represents the actual value of the object, rather than its location.
// The DW_OP_stack_value operation terminates the expression.
bool dw_op_stack_value(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{

    dwarf_value* v = stack->get();
    if(v) {
        v->type  = DWARF_TYPE_GENERIC;
        return true;
    }

    return false;
}

// The DW_OP_call_frame_cfa operation pushes the value of the CFA, obtained from the Call Frame Information (see Section 6.4 on page 171).
bool dw_op_call_frame_cfa(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
    // since in signal handler we are know SP value, just push it to DWARF stack
    unw_word_t sp;
    if(unw_get_reg(&stack->ctx->cursor, UNW_REG_SP, &sp)) {
        return false;
    }

    stack->push(&sp, sizeof(sp), DWARF_TYPE_MEMORY_LOC | DWARF_TYPE_GENERIC);

    return true;
}

// The DW_OP_fbreg operation provides a signed LEB128 offset from the address specified by the location description in the DW_AT_frame_base
// attribute of the current function. This is typically a stack pointer register plus or minus some offset
bool dw_op_fbreg(dwarf_stack* stack, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
    // since in signal handler we are know SP value, just use it as DW_AT_frame_base
    unw_word_t sp;
    if(unw_get_reg(&stack->ctx->cursor, UNW_REG_SP, &sp)) {
        return false;
    }

    int64_t off = decode_sleb128((unsigned char*)&op1);
    sp += off;

    stack->push(&sp, sizeof(sp), DWARF_TYPE_MEMORY_LOC | DWARF_TYPE_GENERIC);

    return true;
}

// DWARF Operations to code & name mapping
dwarf_op_map op_map[] = {
		{DW_OP_addr, 		"DW_OP_addr", 		dw_op_addr},
		{DW_OP_deref, 		"DW_OP_deref", 		dw_op_deref},
		// Constant operations
		{DW_OP_const1u, 	"DW_OP_const1u", 	dw_op_const_x_u},
		{DW_OP_const1s, 	"DW_OP_const1s", 	dw_op_const_x_s},
		{DW_OP_const2u, 	"DW_OP_const2u", 	dw_op_const_x_u},
		{DW_OP_const2s, 	"DW_OP_const2s", 	dw_op_const_x_s},
		{DW_OP_const4u, 	"DW_OP_const4u", 	dw_op_const_x_u},
		{DW_OP_const4s, 	"DW_OP_const4s", 	dw_op_const_x_s},
		{DW_OP_const8u, 	"DW_OP_const8u", 	dw_op_const_x_u},
		{DW_OP_const8s, 	"DW_OP_const8s", 	dw_op_const_x_s},
		{DW_OP_constu,  	"DW_OP_constu",  	dw_op_constu},
		{DW_OP_consts,  	"DW_OP_consts",  	dw_op_consts},
		// DWARF expression stack operations
		{DW_OP_dup,     	"DW_OP_dup",     	dw_op_dup},
		{DW_OP_drop,    	"DW_OP_drop",    	dw_op_drop},
		{DW_OP_over,    	"DW_OP_over",    	dw_op_over},
		{DW_OP_pick,		"DW_OP_pick",		dw_op_pick},
		{DW_OP_swap,		"DW_OP_swap",		dw_op_swap},
		{DW_OP_rot,			"DW_OP_rot",		dw_op_rot},
		{DW_OP_xderef, 		"DW_OP_xderef",		dw_op_notimpl},
		// Arithmetic and Logical Operations
		{DW_OP_abs,			"DW_OP_abs",		dw_op_abs},
		{DW_OP_and, 		"DW_OP_and",		dw_op_and},
		{DW_OP_div,			"DW_OP_div",		dw_op_div},
		{DW_OP_minus,		"DW_OP_minus",		dw_op_minus},
		{DW_OP_mod,			"DW_OP_mod",		dw_op_mod},
		{DW_OP_mul,			"DW_OP_mul",		dw_op_mul},
		{DW_OP_neg, 		"DW_OP_neg",		dw_op_neg},
		{DW_OP_not,			"DW_OP_not", 		dw_op_not},
		{DW_OP_or,			"DW_OP_or",			dw_op_or},
		{DW_OP_plus,		"DW_OP_plus",		dw_op_plus},
		{DW_OP_plus_uconst, "DW_OP_plus_uconst",dw_op_plus_uconst},
		// not implemented for now
		{DW_OP_shl,			"DW_OP_shl",		dw_op_notimpl},
		{DW_OP_shr,			"DW_OP_shr", 		dw_op_notimpl},
		{DW_OP_shra,		"DW_OP_shra",		dw_op_notimpl},
		{DW_OP_xor,			"DW_OP_xor",		dw_op_notimpl},
		{DW_OP_bra,			"DW_OP_bra",		dw_op_notimpl},
		{DW_OP_eq,			"DW_OP_eq",			dw_op_notimpl},
		{DW_OP_ge,			"DW_OP_ge",			dw_op_notimpl},
		{DW_OP_gt, 			"DW_OP_gt",			dw_op_notimpl},
		{DW_OP_le,			"DW_OP_le",			dw_op_notimpl},
		{DW_OP_lt,			"DW_OP_lt",			dw_op_notimpl},
		{DW_OP_ne,			"DW_OP_ne",			dw_op_notimpl},
		{DW_OP_skip,		"DW_OP_skip",		dw_op_notimpl},
		// DWARF5 2.5.1.1 Literal Encodings
		{DW_OP_lit0,		"DW_OP_lit0",		dw_op_lit_x},
		{DW_OP_lit1,		"DW_OP_lit1",		dw_op_lit_x},
		{DW_OP_lit2,		"DW_OP_lit2",		dw_op_lit_x},
		{DW_OP_lit3,		"DW_OP_lit3",		dw_op_lit_x},
		{DW_OP_lit4,		"DW_OP_lit4",		dw_op_lit_x},
		{DW_OP_lit5,		"DW_OP_lit5",		dw_op_lit_x},
		{DW_OP_lit6,		"DW_OP_lit6",		dw_op_lit_x},
		{DW_OP_lit7,		"DW_OP_lit7",		dw_op_lit_x},
		{DW_OP_lit8,		"DW_OP_lit8",		dw_op_lit_x},
		{DW_OP_lit9,		"DW_OP_lit9",		dw_op_lit_x},
		{DW_OP_lit10,		"DW_OP_lit10",		dw_op_lit_x},
		{DW_OP_lit11,		"DW_OP_lit11",		dw_op_lit_x},
		{DW_OP_lit12,		"DW_OP_lit12",		dw_op_lit_x},
		{DW_OP_lit13,		"DW_OP_lit13",		dw_op_lit_x},
		{DW_OP_lit14,		"DW_OP_lit14",		dw_op_lit_x},
		{DW_OP_lit15,		"DW_OP_lit15",		dw_op_lit_x},
		{DW_OP_lit16,		"DW_OP_lit16",		dw_op_lit_x},
		{DW_OP_lit17,		"DW_OP_lit17",		dw_op_lit_x},
		{DW_OP_lit18,		"DW_OP_lit18",		dw_op_lit_x},
		{DW_OP_lit19,		"DW_OP_lit19",		dw_op_lit_x},
		{DW_OP_lit20,		"DW_OP_lit20",		dw_op_lit_x},
		{DW_OP_lit21,		"DW_OP_lit21",		dw_op_lit_x},
		{DW_OP_lit22,		"DW_OP_lit22",		dw_op_lit_x},
		{DW_OP_lit23,		"DW_OP_lit23",		dw_op_lit_x},
		{DW_OP_lit24,		"DW_OP_lit24",		dw_op_lit_x},
		{DW_OP_lit25,		"DW_OP_lit25",		dw_op_lit_x},
		{DW_OP_lit26,		"DW_OP_lit26",		dw_op_lit_x},
		{DW_OP_lit27,		"DW_OP_lit27",		dw_op_lit_x},
		{DW_OP_lit28,		"DW_OP_lit28",		dw_op_lit_x},
		{DW_OP_lit29,		"DW_OP_lit29",		dw_op_lit_x},
		{DW_OP_lit30,		"DW_OP_lit30",		dw_op_lit_x},
		{DW_OP_lit31,		"DW_OP_lit31",		dw_op_lit_x},
		// Register location descriptions. I.e. register containing the value
		// GP Registers
		{DW_OP_reg0, 		"DW_OP_reg0",		dw_op_reg_x},
		{DW_OP_reg1, 		"DW_OP_reg1",		dw_op_reg_x},
		{DW_OP_reg2, 		"DW_OP_reg2",		dw_op_reg_x},
		{DW_OP_reg3, 		"DW_OP_reg3",		dw_op_reg_x},
		{DW_OP_reg4, 		"DW_OP_reg4",		dw_op_reg_x},
		{DW_OP_reg5, 		"DW_OP_reg5",		dw_op_reg_x},
		{DW_OP_reg6, 		"DW_OP_reg6",		dw_op_reg_x},
		{DW_OP_reg7, 		"DW_OP_reg7",		dw_op_reg_x},
		// Extended GP Registers
		{DW_OP_reg8, 		"DW_OP_reg8",		dw_op_reg_x},
		{DW_OP_reg9, 		"DW_OP_reg9",		dw_op_reg_x},
		{DW_OP_reg10, 		"DW_OP_reg10",		dw_op_reg_x},
		{DW_OP_reg11, 		"DW_OP_reg11",		dw_op_reg_x},
		{DW_OP_reg12, 		"DW_OP_reg12",		dw_op_reg_x},
		{DW_OP_reg13, 		"DW_OP_reg13",		dw_op_reg_x},
		{DW_OP_reg14, 		"DW_OP_reg14",		dw_op_reg_x},
		{DW_OP_reg15, 		"DW_OP_reg15",		dw_op_reg_x},
		{DW_OP_reg16, 		"DW_OP_reg16",		dw_op_reg_x}, // Return Address (RA) mapped to RIP
		// SSE Vector Registers
		{DW_OP_reg17, 		"DW_OP_reg17",		dw_op_reg_x},
		{DW_OP_reg18, 		"DW_OP_reg18",		dw_op_reg_x},
		{DW_OP_reg19, 		"DW_OP_reg19",		dw_op_reg_x},
		{DW_OP_reg20, 		"DW_OP_reg20",		dw_op_reg_x},
		{DW_OP_reg21, 		"DW_OP_reg21",		dw_op_reg_x},
		{DW_OP_reg22, 		"DW_OP_reg22",		dw_op_reg_x},
		{DW_OP_reg23, 		"DW_OP_reg23",		dw_op_reg_x},
		{DW_OP_reg24, 		"DW_OP_reg24",		dw_op_reg_x},
		{DW_OP_reg25, 		"DW_OP_reg25",		dw_op_reg_x},
		{DW_OP_reg26, 		"DW_OP_reg26",		dw_op_reg_x},
		{DW_OP_reg27, 		"DW_OP_reg27",		dw_op_reg_x},
		{DW_OP_reg28, 		"DW_OP_reg28",		dw_op_reg_x},
		{DW_OP_reg29, 		"DW_OP_reg29",		dw_op_reg_x},
		{DW_OP_reg30, 		"DW_OP_reg30",		dw_op_reg_x},
		{DW_OP_reg31, 		"DW_OP_reg31",		dw_op_reg_x},
		// Register values. I.e. appropriate register value + offset of operation is pushed onto the stack
		// GP Registers
		{DW_OP_breg0,		"DW_OP_breg0", 		dw_op_breg_x},
		{DW_OP_breg1,		"DW_OP_breg1", 		dw_op_breg_x},
		{DW_OP_breg2, 		"DW_OP_breg2", 		dw_op_breg_x},
		{DW_OP_breg3, 		"DW_OP_breg3", 		dw_op_breg_x},
		{DW_OP_breg4, 		"DW_OP_breg4", 		dw_op_breg_x},
		{DW_OP_breg5, 		"DW_OP_breg5", 		dw_op_breg_x},
		{DW_OP_breg6, 		"DW_OP_breg6", 		dw_op_breg_x},
		{DW_OP_breg7, 		"DW_OP_breg7", 		dw_op_breg_x},
		// Extended GP Registers
		{DW_OP_breg8, 		"DW_OP_breg8", 		dw_op_breg_x},
		{DW_OP_breg9, 		"DW_OP_breg9", 		dw_op_breg_x},
		{DW_OP_breg10, 		"DW_OP_breg10", 	dw_op_breg_x},
		{DW_OP_breg11, 		"DW_OP_breg11", 	dw_op_breg_x},
		{DW_OP_breg12, 		"DW_OP_breg12", 	dw_op_breg_x},
		{DW_OP_breg13, 		"DW_OP_breg13", 	dw_op_breg_x},
		{DW_OP_breg14, 		"DW_OP_breg14", 	dw_op_breg_x},
		{DW_OP_breg15, 		"DW_OP_breg15", 	dw_op_breg_x},
		{DW_OP_breg16, 		"DW_OP_breg16", 	dw_op_breg_x}, // Return Address (RA) mapped to RIP
		// SSE Vector Registers
		{DW_OP_breg17, 		"DW_OP_breg17", 	dw_op_breg_x},
		{DW_OP_breg18, 		"DW_OP_breg18", 	dw_op_breg_x},
		{DW_OP_breg19, 		"DW_OP_breg19", 	dw_op_breg_x},
		{DW_OP_breg20, 		"DW_OP_breg20", 	dw_op_breg_x},
		{DW_OP_breg21, 		"DW_OP_breg21", 	dw_op_breg_x},
		{DW_OP_breg22, 		"DW_OP_breg22", 	dw_op_breg_x},
		{DW_OP_breg23, 		"DW_OP_breg23", 	dw_op_breg_x},
		{DW_OP_breg24, 		"DW_OP_breg24", 	dw_op_breg_x},
		{DW_OP_breg25, 		"DW_OP_breg25", 	dw_op_breg_x},
		{DW_OP_breg26, 		"DW_OP_breg26", 	dw_op_breg_x},
		{DW_OP_breg27, 		"DW_OP_breg27", 	dw_op_breg_x},
		{DW_OP_breg28, 		"DW_OP_breg28", 	dw_op_breg_x},
		{DW_OP_breg29, 		"DW_OP_breg29", 	dw_op_breg_x},
		{DW_OP_breg30, 		"DW_OP_breg30", 	dw_op_breg_x},
		{DW_OP_breg31, 		"DW_OP_breg31", 	dw_op_breg_x},
		// special register-related commands
		{DW_OP_regx, 		"DW_OP_regx",		dw_op_reg_x},   // 1st operand register name
		{DW_OP_fbreg, 		"DW_OP_fbreg", 		dw_op_fbreg},   // base is Frame base register there
		{DW_OP_bregx,		"DW_OP_bregx", 		dw_op_breg_x},  // base is value of 1st operand's register

		// not implemented
		{DW_OP_piece,               "DW_OP_piece",              dw_op_notimpl},
		{DW_OP_deref_size,          "DW_OP_deref_size",         dw_op_notimpl},
		{DW_OP_xderef_size,         "DW_OP_xderef_size",        dw_op_notimpl},
		{DW_OP_nop,                 "DW_OP_nop",                dw_op_notimpl},
		{DW_OP_push_object_address, "DW_OP_push_object_address",dw_op_notimpl},
		{DW_OP_call2,               "DW_OP_call2",              dw_op_notimpl},
		{DW_OP_call4,               "DW_OP_call4",              dw_op_notimpl},
		{DW_OP_call_ref,            "DW_OP_call_ref",           dw_op_notimpl},
		{DW_OP_form_tls_address,    "DW_OP_form_tls_address",   dw_op_notimpl},
		{DW_OP_call_frame_cfa,      "DW_OP_call_frame_cfa",     dw_op_call_frame_cfa},
		{DW_OP_bit_piece,           "DW_OP_bit_piece",          dw_op_notimpl},
		{DW_OP_implicit_value,      "DW_OP_implicit_value",     dw_op_notimpl},
		{DW_OP_stack_value,         "DW_OP_stack_value",        dw_op_stack_value},
		// in fact, implementation is at upper layer since this operation contains sub-expression
		{DW_OP_GNU_entry_value, "DW_OP_GNU_entry_value",        dw_op_notimpl},
};

int find_regnum(uint32_t op)
{
    for(uint32_t i = 0; i < sizeof(reg_map) / sizeof(dwarf_reg_map); ++i) {
        if(reg_map[i].op_num == op) {
            return reg_map[i].regno;
            return true;
        }
    }

    return -1;
}

const dwarf_op_map* find_op_map(int op)
{
	for(uint32_t i = 0; i < sizeof(op_map) / sizeof(dwarf_op_map); ++i) {
		if(op_map[i].op_num == op) {
			return &op_map[i];
		}
	}

	return NULL;
}

bool __dwarf_stack::get_value(uint64_t& value)
{
    if(!Size()) {
        return false;
    }

    value = 0;
    dwarf_value* v = get();
    v->get_uint(value);
    if(v->type & DWARF_TYPE_REGISTER_LOC) {
        // dereference register location
        uint64_t regno = value;
        if(unw_get_reg(&ctx->cursor, regno, &value)) {
            ctx->log(SEVERITY_ERROR, "Failed to get value of register 0x%lX", regno);
            return false;
        }
    }

    return true;
}

bool __dwarf_stack::calc_expression(Dwarf_Op *exprs, int expr_len, Dwarf_Attribute* attr)
{
    clear();
    for (int i = 0; i < expr_len; i++) {
        const dwarf_op_map* map = find_op_map(exprs[i].atom);
        if(!map) {
            ctx->log(SEVERITY_ERROR, "Unknown operation type 0x%hhX(0x%lX, 0x%lX)", exprs[i].atom, exprs[i].number, exprs[i].number2);
            return false;
        }

        dwarf_value* v = get();
        // dereference register location there if it is not last in stack
        if(v && (v->type & DWARF_TYPE_REGISTER_LOC)) {
            unw_word_t value = 0;
            uint64_t regno = *((uint64_t*)v->value);
            if(unw_get_reg(&ctx->cursor, regno, &value)) {
                ctx->log(SEVERITY_ERROR, "Failed to ger value of register 0x%lX", regno);
                return false;
            }
            v->replace(&value, sizeof(value), DWARF_TYPE_GENERIC);
        }

        // handle there because it contains sub-expression of Location
        if(map->op_num == DW_OP_GNU_entry_value) {
            // This opcode has two operands, the first one is uleb128 length and the second is block of that length, containing either a
            // simple register or DWARF expression
            Dwarf_Attribute attr_mem;
            if(!dwarf_getlocation_attr(attr, exprs, &attr_mem)) {
                Dwarf_Op *expr;
                size_t exprlen;
                if (dwarf_getlocation(&attr_mem, &expr, &exprlen) == 0) {
                    //offset += print_expr_block (expr, exprlen, buff + offset, buff_size - offset, &attr_mem);
                    //offset += snprintf(buff + offset, buff_size - offset, ") ");
                    if(!calc_expression(expr, exprlen, &attr_mem)) {
                        ctx->log(SEVERITY_ERROR, "Failed to calculate sub-expression for operation %s(0x%lX, 0x%lX)", map->op_name, exprs[i].number, exprs[i].number2);
                        return false;
                    }
                    continue;
                } else {
                    ctx->log(SEVERITY_ERROR, "Failed to get DW_OP_GNU_entry_value attr location");
                }
            } else {
                ctx->log(SEVERITY_ERROR, "Failed to get DW_OP_GNU_entry_value attr expression");
            }
        }

        if(!map->operation(this, map, exprs[i].number, exprs[i].number2)) {
            ctx->log(SEVERITY_ERROR, "Failed to calculate %s(0x%lX, 0x%lX) operation", map->op_name, exprs[i].number, exprs[i].number2);
            return false;
        }

    }

    return true;
}
