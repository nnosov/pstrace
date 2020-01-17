/*
 * sysutils.cpp
 *
 *  Created on: Dec 28, 2019
 *      Author: nnosov
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#include <elfutils/libdwfl.h>
#include <cxxabi.h>
#include <execinfo.h>
#include <dwarf.h>
#include <inttypes.h>

#include "sysutils.h"
#include "logger/log.h"

#define USE_LIBUNWIND

#ifdef USE_LIBUNWIND
#include <libunwind.h>
#endif

#include "common.h"
#include "dwarf_operations.h"

// dwfl_addrsegment() possibly can be used to check address validity
// dwarf_getattrs() allows to enumerate all DIE attributes
// dwarf_getfuncs() allows to enumerate functions within CU

typedef struct __reginfo {
	__reginfo() {
		regname[0] = 0;
		regno = -1;
	}
	char regname[32];
	int regno;
} reginfo;

int regname_callback (void *arg, int regno, const char *setname, const char *prefix, const char *regname, int bits, int type)
{
	reginfo* info = (reginfo*)arg;
	if(info->regno == regno) {
		snprintf(info->regname, sizeof(info->regname), "%s %s%s", setname, prefix, regname);
	}

	return 0;
}

bool handle_location(pst_context* ctx, Dwarf_Attribute* attr, dwarf_stack& stack, Dwarf_Addr pc, char* str, uint32_t strsize)
{
    str[0] = 0;
    Dwarf_Addr offset = pc - ctx->base_addr;

    if(dwarf_hasform(attr, DW_FORM_exprloc)) {
        // Location expression (exprloc class of location in DWARF terms)
        Dwarf_Op *expr;
        size_t exprlen;
        if(dwarf_getlocation(attr, &expr, &exprlen) == 0) {
            ctx->print_expr_block (expr, exprlen, str, strsize, attr);
            if(stack.calc_expression(expr, exprlen, attr)) {
                return true;
            }
        }
    } else if(dwarf_hasform(attr, DW_FORM_sec_offset)) {
        // Location list (loclist class of location in DWARF terms)
        Dwarf_Addr base, start, end;
        ptrdiff_t off = 0;
        Dwarf_Op *expr;
        size_t exprlen;

        // handle list of possible locations of parameter
        for(int i = 0; (off = dwarf_getlocations (attr, off, &base, &start, &end, &expr, &exprlen)) > 0; ++i) {
            ctx->print_expr_block (expr, exprlen, str, strsize, attr);
            if(offset >= start && offset <= end) {
                // actual location, try to calculate Location expression
                if(stack.calc_expression(expr, exprlen, attr)) {
                    return true;
                }
            } else {
                // Location skipped due to don't match current PC offset
                ctx->log(SEVERITY_DEBUG, "Skip Location list expression: [%d] (low_offset: 0x%" PRIx64 ", high_offset: 0x%" PRIx64 "), \"%s\"", i, start, end, str);
            }
        }

    } else {
        ctx->log(SEVERITY_WARNING, "Unknown location attribute form = 0x%X, code = 0x%X, ", attr->form, attr->code);
    }

    return false;
}

/*
int reginfo_callback (void *arg, int regno, const char *setname, const char *prefix, const char *regname, int bits, int type)
{
	reginfo* info = (reginfo*)arg;

	//logger->Log(SEVERITY_DEBUG, "%s: info_reg = 0x%hhX, reg = 0x%hhX", __FUNCTION__, info->regno, regno);
	if(info->regno == regno) {
		snprintf(info->regname, sizeof(info->regname), "%s %s%s", setname, prefix, regname);
		Dwarf_Op ops_mem[3];
		Dwarf_Op* ops;
		size_t nops;
		int ret = dwarf_frame_register(frame, info->regno, ops_mem, &ops, &nops);
		if(ret != 0) {
			logger->Log(SEVERITY_DEBUG, "Failed to get CFI expression for register %s", info->regname);
			return 0;
		}
		if(nops == 0 && ops == ops_mem) {
			logger->Log(SEVERITY_DEBUG, "CFI expression for register %s is UNDEFINED", info->regname);
			return 0;
		}
		if(nops == 0 && ops == NULL) {
			logger->Log(SEVERITY_DEBUG, "CFI expression for register %s is SAME VALUE", info->regname);
			return 0;
		}
		logger->Log(SEVERITY_DEBUG, "Found CFI expression for register %s, number of expressions: %d", info->regname, nops);
		for(size_t i = 0; i < nops; ++i) {
			logger->Log(SEVERITY_DEBUG, "\t[%d] operation: 0x%hhX, operand1: 0x%lX, operand2: 0x%lx, offset: 0x%lX", i, ops[i].atom, ops[i].number, ops[i].number2, ops[i].offset);
		}
	}

	return 0;
}
*/

/*
void print_framereg(int regno)
{
	Dwarf_Op ops_mem[3];
	Dwarf_Op* ops;
	size_t nops;
	int ret = dwarf_frame_register(frame, regno, ops_mem, &ops, &nops);
	if(ret != 0) {
		logger->Log(SEVERITY_DEBUG, "Failed to get CFI expression for register 0x%hhX", regno);
		return;
	}
	if(nops == 0 && ops == ops_mem) {
		logger->Log(SEVERITY_DEBUG, "CFI expression for register 0x%hhX is UNDEFINED", regno);
		return;
	}
	if(nops == 0 && ops == NULL) {
		logger->Log(SEVERITY_DEBUG, "CFI expression for register 0x%hhX is SAME VALUE", regno);
		return;
	}
	logger->Log(SEVERITY_DEBUG, "Found CFI expression for register 0x%hhX, number of expressions: %d", regno, nops);
	for(size_t i = 0; i < nops; ++i) {
		logger->Log(SEVERITY_DEBUG, "\t[%d] operation: 0x%hhX, operand1: 0x%lX, operand2: 0x%lx, offset: 0x%lX", i, ops[i].atom, ops[i].number, ops[i].number2, ops[i].offset);
	}
}
*/

bool __pst_parameter::print_dwarf()
{
    if(types.Size()) {
        if(!is_return) {
            if(has_value) {
                ctx->print("%s %s = 0x%lX", next_type(NULL)->name.c_str(), name.c_str(), value);
            } else {
                ctx->print("%s %s = <undefined>", next_type(NULL)->name.c_str(), name.c_str());
            }
        } else {
            ctx->print("%s", next_type(NULL)->name.c_str());
        }
    } else {
        if(has_value) {
            ctx->print("%s = 0x%lX", name.c_str(), value);
        } else {
            ctx->print("%s = <undefined>", name.c_str());
        }
    }
    return true;
}

bool __pst_parameter::handle_type(Dwarf_Attribute* param)
{
	Dwarf_Attribute attr_mem;
	Dwarf_Attribute* attr;

	// get DIE of return type
	Dwarf_Die ret_die;

	if(!dwarf_formref_die(param, &ret_die)) {
		ctx->log(SEVERITY_ERROR, "Failed to get parameter DIE");
		return false;
	}

	switch (dwarf_tag(&ret_die)) {
		case DW_TAG_base_type: {
			// get Size attribute and it's value
			Dwarf_Word size = 0;
			attr = dwarf_attr(&ret_die, DW_AT_byte_size, &attr_mem);
			if(attr) {
				dwarf_formudata(attr, &size);
			}
			ctx->log(SEVERITY_DEBUG, "base type '%s'(%lu)", dwarf_diename(&ret_die), size);
			add_type(dwarf_diename(&ret_die), DW_TAG_base_type);
			type = DW_TAG_base_type;
			break;
		}
		case DW_TAG_array_type:
			ctx->log(SEVERITY_DEBUG, "array type");
			add_type("[]", DW_TAG_array_type);
			break;
		case DW_TAG_structure_type:
			ctx->log(SEVERITY_DEBUG, "structure type");
			add_type("struct", DW_TAG_structure_type);
			break;
		case DW_TAG_union_type:
			ctx->log(SEVERITY_DEBUG, "union type");
			add_type("union", DW_TAG_union_type);
			break;
		case DW_TAG_class_type:
			ctx->log(SEVERITY_DEBUG, "class type");
			add_type("class", DW_TAG_class_type);
			break;
		case DW_TAG_pointer_type:
			ctx->log(SEVERITY_DEBUG, "pointer type");
			add_type("*", DW_TAG_pointer_type);
			break;
		case DW_TAG_enumeration_type:
			ctx->log(SEVERITY_DEBUG, "enumeration type");
			add_type("enum", DW_TAG_enumeration_type);
			break;
		case DW_TAG_const_type:
			ctx->log(SEVERITY_DEBUG, "constant type");
			add_type("const", DW_TAG_const_type);
			break;
		case DW_TAG_subroutine_type:
			ctx->log(SEVERITY_DEBUG, "Skipping subroutine type");
			break;
		case DW_TAG_typedef:
			ctx->log(SEVERITY_DEBUG, "typedef '%s' type", dwarf_diename(&ret_die));
			add_type(dwarf_diename(&ret_die), DW_TAG_typedef);
			break;
		default:
			ctx->log(SEVERITY_WARNING, "Unknown 0x%X tag type", dwarf_tag(&ret_die));
			break;
	}

	attr = dwarf_attr(&ret_die, DW_AT_type, &attr_mem);
	if(attr) {
		return handle_type(attr);
	}

	return true;
}

pst_type* __pst_parameter::add_type(const char* name, int type)
{
    pst_type* t = new pst_type(name, type);
    types.InsertLast(t);

    return t;
}

void __pst_parameter::clear()
{
    for(pst_type* t = (pst_type*)types.First(); t; t = (pst_type*)types.First()) {
        types.Remove(t);
        delete t;
    }
}

pst_type* __pst_parameter::next_type(pst_type* t)
{
    pst_type* next = NULL;
    if(!t) {
        next = (pst_type*)types.First();
    } else {
        next = (pst_type*)types.Next(t);
    }

    return next;
}

bool __pst_parameter::handle_dwarf(Dwarf_Die* result)
{
	die = result;

	Dwarf_Attribute attr_mem;
	Dwarf_Attribute* attr;

	name = dwarf_diename(result);
	is_variable = (dwarf_tag(result) == DW_TAG_variable);

	dwarf_decl_line(result, (int*)&line);
	// Get reference to attribute type of the parameter/variable
	attr = dwarf_attr(result, DW_AT_type, &attr_mem);
	ctx->log(SEVERITY_DEBUG, "Handle '%s' %s", name.c_str(), dwarf_tag(result) == DW_TAG_formal_parameter ? "parameter" : "variable");
	if(attr) {
		handle_type(attr);
	}

	if(dwarf_hasattr(result, DW_AT_location)) {
	    // determine location of parameter in stack/heap or CPU registers
	    attr = dwarf_attr(result, DW_AT_location, &attr_mem);
	    Dwarf_Addr pc;
	    unw_get_reg(&ctx->curr_frame, UNW_REG_IP,  &pc);

	    dwarf_stack stack(ctx);
	    char str[1024]; str[0] = 0;
	    if(handle_location(ctx, attr, stack, pc, str, sizeof(str))) {
	        if(stack.get_value(value)) {
	            ctx->log(SEVERITY_DEBUG, "Parameter Location: %s ==> 0x%lX", str, value);
	            has_value = true;
	        } else {
	            ctx->log(SEVERITY_ERROR, "Failed to get value of calculated DW_AT_location expression: %s", str);
	        }
	    } else {
	        ctx->log(SEVERITY_ERROR, "Failed to calculate DW_AT_location expression: %s", str);
	    }
	} else if(dwarf_hasattr(result, DW_AT_const_value)) {
	    // no locations definitions, value is constant, known by DWARF directly
	    attr = dwarf_attr(result, DW_AT_const_value, &attr_mem);
	    switch (dwarf_whatform(attr)) {
	        case DW_FORM_string:
	            // do nothing for now
	            ctx->log(SEVERITY_WARNING, "Const value form DW_FORM_string is not implemented.");
	            break;
	        case DW_FORM_data1:
	        case DW_FORM_data2:
	        case DW_FORM_data4:
	        case DW_FORM_data8:
                dwarf_formudata(attr, &value);
                has_value = true;
                break;
	        case DW_FORM_sdata:
	            dwarf_formsdata(attr, (int64_t*)&value);
	            has_value = true;
	            break;
	        case DW_FORM_udata:
	            dwarf_formudata(attr, &value);
	            has_value = true;
	            break;
	    }
	    if(has_value) {
	        ctx->log(SEVERITY_DEBUG, "Parameter constant value: 0x%lX", value);
	    }
	}

	//  handle DW_AT_default_value to get information about default value for DW_TAG_formal_parameter type of function

	return true;
}

pst_parameter* __pst_function::add_param()
{
    pst_parameter* p = new pst_parameter(ctx);
    params.InsertLast(p);

    return p;
}

void __pst_function::del_param(pst_parameter* p)
{
    params.Remove(p);
    delete p;
}

void __pst_function::clear()
{
    for(pst_parameter* p = (pst_parameter*)params.First(); p; p = (pst_parameter*)params.First()) {
        params.Remove(p);
        delete p;
    }
}

pst_parameter* __pst_function::next_param(pst_parameter* p)
{
    pst_parameter* next = NULL;
    if(!p) {
        next = (pst_parameter*)params.First();
    } else {
        next = (pst_parameter*)params.Next(p);
    }

    return next;
}

bool __pst_function::print_dwarf()
{
    char* at = NULL;
    if(!asprintf(&at, " at %s:%d, %p", file.c_str(), line, (void*)pc)) {
        return false;
    }
    if(at[4] == ':' && at[5] == '-') {
        free(at);
        if(!asprintf(&at, " at %p", (void*)pc)) {
            return false;
        }
    }
    //ctx->print("%s:%d: ", file.c_str(), line);

    // handle return parameter and be safe if function haven't parameters (for example, dwar info for function is absent)
    pst_parameter* param = next_param(NULL);
    if(param && param->is_return) {
        // print return value type, function name and start list of parameters
        param->print_dwarf();
        ctx->print(" %s(", name.c_str());
        param = next_param(param);
    } else {
        ctx->print("%s(", name.c_str());
    }

    bool first = true; bool start_variable = false;
    for(; param; param = next_param(param)) {
        if(param->is_return) {
            // print return value type, function name and start list of parameters
            param->print_dwarf();
            ctx->print(" %s(", name.c_str());
            continue;
        }

        if(param->is_variable) {
            if(!start_variable) {
                ctx->print(")%s\n", at);
                ctx->print("{\n");
                start_variable = true;
            }
            if(param->line) {
                ctx->print("%04u:   ", param->line);
            } else {
                ctx->print("        ");
            }
            param->print_dwarf();
            ctx->print(";\n");
        } else {
            if(first) {
                first = false;
            } else {
                ctx->print(", ");
            }
            param->print_dwarf();
        }
    }

    if(!start_variable) {
        ctx->print(");%s\n", at);
    } else {
        ctx->print("}\n");
    }

    free(at);
    return true;
}

bool __pst_function::handle_lexical_block(Dwarf_Die* result)
{
    uint64_t lowpc = 0, highpc = 0; const char* origin_name = "";
    dwarf_lowpc(result, &lowpc);
    dwarf_highpc(result, &lowpc);

    Dwarf_Attribute attr_mem;
    Dwarf_Die origin;
    if(dwarf_hasattr (result, DW_AT_abstract_origin) && dwarf_formref_die (dwarf_attr (result, DW_AT_abstract_origin, &attr_mem), &origin) != NULL) {
        origin_name = dwarf_diename(&origin);
        Dwarf_Die child;
        if(dwarf_child (&origin, &child) == 0) {
            do {
                switch (dwarf_tag (&child))
                {
                    case DW_TAG_variable:
                    case DW_TAG_formal_parameter:
                        ctx->log(SEVERITY_DEBUG, "Abstract origin: %s('%s')", dwarf_diename(&origin), dwarf_diename (&child));
                        break;
                    default:
                        break;
                }
            } while (dwarf_siblingof (&child, &child) == 0);
        }
    }
    const char* die_name = "";
    if(dwarf_diename(result)) {
        die_name = dwarf_diename(result);
    }
    ctx->log(SEVERITY_DEBUG, "Lexical block with name '%s', tag 0x%X and origin '%s' found. lowpc = 0x%lX, highpc = 0x%lX", die_name, dwarf_tag (result), origin_name, lowpc, highpc);
    Dwarf_Die child;
    if(dwarf_child (result, &child) == 0) {
        do {
            switch (dwarf_tag (&child)) {
                case DW_TAG_lexical_block:
                    handle_lexical_block(&child);
                    break;
                case DW_TAG_variable: {
                    pst_parameter* param = add_param();
                    if(!param->handle_dwarf(&child)) {
                        del_param(param);
                    }
                    break;
                }
                case DW_TAG_GNU_call_site:
                    handle_call_site(&child);
                    break;
                case DW_TAG_inlined_subroutine:
                    ctx->log(SEVERITY_DEBUG, "Skipping Lexical block tag 'DW_TAG_inlined_subroutine'");
                    break;
                default:
                    ctx->log(SEVERITY_DEBUG, "Unknown Lexical block tag 0x%X", dwarf_tag(&child));
                    break;
            }
        }while (dwarf_siblingof (&child, &child) == 0);
    }

    return true;
}

// DW_AT_low_pc should point to the offset from process base address which is actually PC of current function, usually.
// further handle DW_AT_abstract_origin attribute of DW_TAG_GNU_call_site DIE to determine what DIE is referenced by it.
// probably by invoke by:
// Dwarf_Die *scopes;
// int n = dwarf_getscopes_die (funcdie, &scopes); // where 'n' is the number of scopes
// if (n <= 0) -> FAILURE
// see handle_function() in elfutils/tests/funcscopes.c -> handle_function() -> print_vars()
// DW_TAG_GNU_call_site_parameter is defined under child DIE of DW_TAG_GNU_call_site and defines value of subroutine before calling it
// relates to DW_OP_GNU_entry_value() handling in callee function to determine the value of an argument/variable of the callee
// get DIE of return type
bool __pst_function::handle_call_site(Dwarf_Die* result)
{
    Dwarf_Die origin;
    Dwarf_Attribute attr_mem;
    Dwarf_Attribute* attr;

    if(dwarf_hasattr (result, DW_AT_abstract_origin) && dwarf_formref_die (dwarf_attr (result, DW_AT_abstract_origin, &attr_mem), &origin) != NULL) {
        Dwarf_Die child;
        if(dwarf_child (&origin, &child) == 0) {
            do {
                switch (dwarf_tag (&child))
                {
                    case DW_TAG_variable:
                    case DW_TAG_formal_parameter:
                        ctx->log(SEVERITY_DEBUG, "Abstract origin: %s('%s')", dwarf_diename(&origin), dwarf_diename (&child));
                        break;
                    default:
                        break;
                }
            } while (dwarf_siblingof (&child, &child) == 0);
        }
    }
    Dwarf_Die child;
    if(dwarf_child (result, &child) == 0) {
        do {
            switch (dwarf_tag (&child))
            {
                case DW_TAG_GNU_call_site_parameter: {
                    Dwarf_Addr pc;
                    unw_get_reg(&cursor, UNW_REG_IP,  &pc);

                    dwarf_stack stack(ctx); char str[1024]; uint64_t value;
                    if(dwarf_hasattr(&child, DW_AT_location)) {
                        // handle location expression here
                        // determine location of parameter in stack/heap or CPU registers
                        attr = dwarf_attr(&child, DW_AT_location, &attr_mem);
                        if(handle_location(ctx, attr, stack, pc, str, sizeof(str))) {
                            if(stack.get_value(value)) {
                                ctx->log(SEVERITY_DEBUG, "DW_TAG_GNU_call_site_parameter Location: \"%s\" ==> 0x%lX", str, value);
                            } else {
                                ctx->log(SEVERITY_ERROR, "Failed to get value of calculated DW_AT_location expression: %s", str);
                            }
                        } else {
                            ctx->log(SEVERITY_ERROR, "Failed to calculate DW_AT_location expression: %s", str);
                        }
                    }
                    if(dwarf_hasattr(&child, DW_AT_GNU_call_site_value)) {
                        // handle value expression here
                        attr = dwarf_attr(&child, DW_AT_GNU_call_site_value, &attr_mem);
                        if(handle_location(ctx, attr, stack, pc, str, sizeof(str))) {
                            if(stack.get_value(value)) {
                                ctx->log(SEVERITY_DEBUG, "DW_AT_GNU_call_site_value Location:\"%s\" ==> 0x%lX", str, value);
                            } else {
                                ctx->log(SEVERITY_ERROR, "Failed to get value of calculated DW_AT_location expression: %s", str);
                            }
                        } else {
                            ctx->log(SEVERITY_ERROR, "Failed to calculate DW_AT_location expression: %s", str);
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        } while (dwarf_siblingof (&child, &child) == 0);
    }

    return true;
}

bool __pst_function::handle_dwarf(Dwarf_Die* d)
{
	die = d;
	ctx->curr_frame = cursor;

	Dwarf_Attribute attr_mem;
	Dwarf_Attribute* attr;

	// get list of offsets from process base address of continuous memory ranges where function's code resides
//	if(dwarf_haspc(d, pc)) {
		dwarf_lowpc(d, &lowpc);
		dwarf_highpc(d, &highpc);
//	} else {
//		ctx->log(SEVERITY_ERROR, "Function's '%s' DIE hasn't definitions of memory offsets of function's code", dwarf_diename(d));
//		return false;
//	}

    unw_proc_info_t info;
    unw_get_proc_info(&cursor, &info);
	unw_word_t sp;
	unw_get_reg(&cursor, UNW_REG_SP, &sp);

    ctx->log(SEVERITY_DEBUG, "=====> %s(...), PC = 0x%lX, LOW_PC = 0x%lX, HIGH_PC = 0x%lX, offset from base address: 0x%lX, BASE_PC = 0x%lX, offset from start of function: 0x%lX, stack address: 0x%lX",
    		dwarf_diename(d), pc, lowpc, highpc, pc - ctx->base_addr, info.start_ip, info.start_ip - ctx->base_addr, sp);

	// determine function's stack frame base
	attr = dwarf_attr(die, DW_AT_frame_base, &attr_mem);
	if(attr) {
		if(dwarf_hasform(attr, DW_FORM_exprloc)) {
			Dwarf_Op *expr;
			size_t exprlen;
			if (dwarf_getlocation (attr, &expr, &exprlen) == 0) {
				char str[1024]; str[0] = 0;
				ctx->print_expr_block (expr, exprlen, str, sizeof(str), attr);
				dwarf_stack stack(ctx);
				if(stack.calc_expression(expr, exprlen, attr)) {
				    uint64_t value;
				    if(stack.get_value(value)) {
				        ctx->log(SEVERITY_DEBUG, "DW_AT_framebase expression: \"%s\" ==> 0x%lX", str, value);
				    } else {
				        ctx->log(SEVERITY_ERROR, "Failed to get value of calculated DW_AT_framebase expression: %s", str);
				    }
				} else {
				    ctx->log(SEVERITY_ERROR, "Failed to calculate DW_AT_framebase expression: %s", str);
				}
			} else {
				ctx->log(SEVERITY_WARNING, "Unknown attribute form = 0x%X, code = 0x%X", attr->form, attr->code);
			}
		}
	}

	// Get reference to return attribute type of the function
	// may be to use dwfl_module_return_value_location() instead
	pst_parameter* ret_p = add_param(); ret_p->is_return = true;
	attr = dwarf_attr(die, DW_AT_type, &attr_mem);
	if(attr) {
		if(!ret_p->handle_type(attr)) {
		    ctx->log(SEVERITY_ERROR, "Failed to handle return parameter type");
			del_param(ret_p);
		}
	} else {
		ret_p->add_type("void", 0);
	}

	Dwarf_Die result;
	if(dwarf_child(die, &result) != 0)
		return false;

	// went through parameters and local variables of the function
	do {

		switch (dwarf_tag(&result)) {
		case DW_TAG_formal_parameter:
		case DW_TAG_variable: {
		    pst_parameter* param = add_param();
			if(!param->handle_dwarf(&result)) {
				del_param(param);
			}

			break;
		}
		case DW_TAG_GNU_call_site:
		    handle_call_site(&result);
	        break;

			//				case DW_TAG_inlined_subroutine:
			//					/* Recurse further down */
			//					HandleFunction(&result);
			//					break;
		case DW_TAG_lexical_block: {
		    handle_lexical_block(&result);
		    break;
		}
		default:
		    ctx->log(SEVERITY_DEBUG, "Unknown TAG of function: 0x%X", dwarf_tag(&result));
			break;
		}
	} while(dwarf_siblingof(&result, &result) == 0);

	return true;
}

bool __pst_function::unwind(Dwfl* dwfl, Dwfl_Module* module, Dwarf_Addr addr)
{
	pc = addr;

	Dwfl_Line *dwline = dwfl_getsrc(dwfl, addr);
	if(dwline != NULL) {
		Dwarf_Addr addr;
		const char* filename = dwfl_lineinfo (dwline, &addr, &line, NULL, NULL, NULL);
		if(filename) {
			const char* str = strrchr(filename, '/');
			if(str && *str != 0) {
				str++;
			} else {
				str = filename;
			}
			file = str;
			ctx->print("%s:%d", str, line);
		} else {
			ctx->print("%p", (void*)addr);
		}
	} else {
		ctx->print("%p", (void*)addr);
	}

	const char* addrname = dwfl_module_addrname(module, addr);
	char* demangle_name = NULL;
	if(addrname) {
		int status;
		demangle_name = abi::__cxa_demangle(addrname, NULL, NULL, &status);
		char* function_name = NULL;
		if(asprintf(&function_name, "%s%s", demangle_name ? demangle_name : addrname, demangle_name ? "" : "()") == -1) {
			ctx->log(SEVERITY_ERROR, "Failed to allocate memory");
			return false;
		}
		ctx->print(" --> %s", function_name);

		char* str = strchr(function_name, '(');
		if(str) {
			*str = 0;
		}
		name = function_name;
		free(function_name);
	}

	if(demangle_name) {
		free(demangle_name);
	}

	return true;
}


bool __pst_handler::get_frame()
{
    // get CFI (Call Frame Information) for current module
    // from handle_cfi()
    Dwarf_Addr mod_bias = 0;
    Dwarf_CFI* cfi = dwfl_module_eh_cfi(module, &mod_bias);
    //Dwarf_CFI* cfi = dwfl_module_dwarf_cfi(module, &mod_bias);
    if(!cfi) {
    	ctx.log(SEVERITY_ERROR, "Cannot find CFI for module");
    	return false;
    }

    // get frame of CFI for address
    int result = dwarf_cfi_addrframe (cfi, addr - mod_bias, &frame);
    if (result == 0) {
    	// get frame information
    	ctx.log(SEVERITY_DEBUG, "Found CFI frame for module");
    	Dwarf_Addr start = addr;
    	Dwarf_Addr end = addr;
    	bool signalp;
    	int ra_regno = dwarf_frame_info (frame, &start, &end, &signalp);
    	if(ra_regno >= 0) {
    		start += mod_bias;
    		end += mod_bias;
    	}
    	ctx.log(SEVERITY_DEBUG, "Per '.eh_frame' info has %#" PRIx64 " => [%#" PRIx64 ", %#" PRIx64 "] in_signal = %s", addr, start, end, signalp ? "true" : "false");
    	if (ra_regno < 0)
    		ctx.log(SEVERITY_DEBUG, "return address register unavailable (%s)", dwarf_errmsg(0));
    	else {
    		reginfo info; info.regno = ra_regno;
    		dwfl_module_register_names(module, regname_callback, &info);
    		ctx.log(SEVERITY_DEBUG, "return address in reg%u%s ==> %s", ra_regno, signalp ? " (signal frame)" : "", info.regname);
    	}

    	// finally get CFA (Canonical Frame Address)
    	// Point cfa_ops to dummy to match print_detail expectations.
    	// (nops == 0 && cfa_ops != NULL => "undefined")
    	Dwarf_Op dummy;
    	Dwarf_Op *cfa_ops = &dummy;
    	size_t cfa_nops;
    	result = dwarf_frame_cfa(frame, &cfa_ops, &cfa_nops);
    	char str[1024]; str[0] = 0;
    	ctx.print_expr_block (cfa_ops, cfa_nops, str, sizeof(str));
    	ctx.log(SEVERITY_INFO, "CFA expression: \"%s\"", str);
    	//print_detail (result, cfa_ops, cfa_nops, mod_bias, "\tCFA ");
    }

    return true;
}

bool __pst_handler::get_dwarf_function(pst_function* fun)
{
    Dwarf_Addr mod_cu = 0;
    // get CU(Compilation Unit) debug definition
    module = dwfl_addrmodule(dwfl, fun->pc);
    Dwarf_Die* cdie = dwfl_module_addrdie(module, fun->pc, &mod_cu);
    //Dwarf_Die* cdie = dwfl_addrdie(dwfl, addr, &mod_bias);
    if(!cdie) {
    	ctx.log(SEVERITY_INFO, "Failed to find DWARF DIE for address %X", fun->pc);
    	return false;
    }

    if(dwarf_tag(cdie) != DW_TAG_compile_unit) {
    	ctx.log(SEVERITY_DEBUG, "Skipping non-cu die. DWARF tag: 0x%X, name = %s", dwarf_tag(cdie), dwarf_diename(cdie));
    	return false;
    }

    Dwarf_Die result;
	if(dwarf_child(cdie, &result)) {
		ctx.log(SEVERITY_ERROR, "No child DIE found for CU %s", dwarf_diename(cdie));
		return false;
	}

	bool nret = false;

	do {
		int tag = dwarf_tag(&result);
		if(tag == DW_TAG_subprogram || tag == DW_TAG_entry_point || tag == DW_TAG_inlined_subroutine) {
		    //ctx.log(SEVERITY_DEBUG, "function die name %s", dwarf_diename(&result));
			if(!strcmp(fun->name.c_str(), dwarf_diename(&result))) {
				return fun->handle_dwarf(&result);
			}
		}
	} while(dwarf_siblingof(&result, &result) == 0);

	return nret;
}

pst_function* __pst_handler::add_function(__pst_function* fun)
{
    pst_function* f = new pst_function(&ctx, fun);
    functions.InsertLast(f);

    return f;
}

void __pst_handler::del_function(pst_function* f)
{
    functions.Remove(f);
    delete f;
}

void __pst_handler::clear()
{
    for(pst_function* f = (pst_function*)functions.First(); f; f = (pst_function*)functions.First()) {
        functions.Remove(f);
        delete f;
    }
}

pst_function* __pst_handler::next_function(pst_function* f)
{
    pst_function* next = NULL;
    if(!f) {
        next = (pst_function*)functions.First();
    } else {
        next = (pst_function*)functions.Next(f);
    }

    return next;
}

bool __pst_handler::handle_dwarf()
{
    Dl_info info;

    for(pst_function* fun = next_function(NULL); fun; fun = next_function(fun)) {
        dladdr((void*)(fun->pc), &info);
        ctx.base_addr = (uint64_t)info.dli_fbase;
        ctx.log(SEVERITY_DEBUG, "Function %s(...): module name: %s, base address: %p", fun->name.c_str(), info.dli_fname, info.dli_fbase);

        ctx.curr_frame = fun->cursor;
        if(fun->parent) {
            ctx.next_frame = fun->parent->cursor;
            ctx.has_nframe = true;
        } else {
            ctx.has_nframe = false;
        }
        get_dwarf_function(fun);
    }

    return true;
}

void __pst_handler::print_dwarf()
{
    ctx.offset = 0;
    ctx.print("DWARF-based stack trace information:\n");
    uint32_t idx = 0;
    for(pst_function* function = next_function(NULL); function; function = next_function(function)) {
        ctx.print("[%-2u] ", idx); idx++;
        function->print_dwarf();
        ctx.print("\n");
    }
}

char *debuginfo_path = NULL;
Dwfl_Callbacks callbacks = {
        .find_elf           = dwfl_linux_proc_find_elf,
        .find_debuginfo     = dwfl_standard_find_debuginfo,
        .section_address    = dwfl_offline_section_address,
        .debuginfo_path     = &debuginfo_path,
};

#include <dlfcn.h>

bool __pst_handler::unwind()
{
#ifdef REG_RIP // x86_64
    caller = (void *) ctx.hcontext->uc_mcontext.gregs[REG_RIP];
#elif defined(REG_EIP) // x86_32
    caller_address = (void *) uctx->uc_mcontext.gregs[REG_EIP]);
#elif defined(__arm__)
    caller_address = (void *) uctx->uc_mcontext.arm_pc);
#elif defined(__aarch64__)
    caller_address = (void *) uctx->uc_mcontext.pc);
#elif defined(__ppc__) || defined(__powerpc) || defined(__powerpc__) || defined(__POWERPC__)
    caller_address = (void *) uctx->uc_mcontext.regs->nip);
#elif defined(__s390x__)
    caller_address = (void *) uctx->uc_mcontext.psw.addr);
#elif defined(__APPLE__) && defined(__x86_64__)
    caller_address = (void *) uctx->uc_mcontext->__ss.__rip);
#else
#   error "unknown architecture!"
#endif
    ctx.offset = 0;

    //handle = dlopen(NULL, RTLD_NOW);
	Dl_info info;
	dladdr(caller, &info);
	ctx.base_addr = (uint64_t)info.dli_fbase;
	ctx.log(SEVERITY_INFO, "Process address information: PC address: %p, base address: %p, object name: %s", caller, info.dli_fbase, info.dli_fname);

    int skipped = 0;
#ifndef USE_LIBUNWIND
    int size = 0;
    void * array[50];
    size = backtrace(array, 50);
    for(int i = 0; i < size && array[i] != caller; ++i, ++skipped);
    ctx.print("Stack trace(caller = %p. Total stack frames: %d, skipped: %d):\n", caller, size, skipped);
#else
    unw_word_t  	pc;

    unw_getcontext(&ctx.context);
    unw_init_local(&ctx.curr_frame, &ctx.context);
    while (unw_step(&ctx.curr_frame) > 0) {
    	unw_get_reg(&ctx.curr_frame, UNW_REG_IP,  &pc);
    	if(pc == (uint64_t)caller) {
    		break;
    	} else {
    		++skipped;
    	}
    }
    ctx.print("Stack trace(caller = %p. Skipped stack frames: %d):\n", caller, skipped);
#endif

    dwfl = dwfl_begin(&callbacks);
    if(dwfl == NULL) {
    	ctx.print("Failed to initialize libdw session for parse stack frames");
        return false;
    }

    if(dwfl_linux_proc_report(dwfl, getpid()) != 0 || dwfl_report_end(dwfl, NULL, NULL) !=0) {
    	ctx.print("Failed to parse debug section of executable");
    	return false;
    }

#ifndef USE_LIBUNWIND
    for (int i = skipped, idx = 0; i < size ; ++i, ++idx) {
        uint64_t pc = (uint64_t)array[i];
    	ctx.print("[%-2d] ", idx);
    	Dwarf_Addr addr = (uintptr_t)array[i];
#else
   ctx.has_nframe = false;
   for (int i = skipped, idx = 0; true ; ++i, ++idx) {
   		unw_get_reg(&ctx.curr_frame, UNW_REG_IP,  &pc);
   		addr = pc;
   		ctx.print("[%-2d] ", idx);
#endif
   		module = dwfl_addrmodule(dwfl, addr);
   		get_frame();
   		pst_function* fun = add_function(next_function(NULL)/* first element */);
   		if(!fun->unwind(dwfl, module, pc)) {
   		    del_function(fun);
   		}
   		ctx.print("\n");
#ifdef USE_LIBUNWIND
   		if(unw_step(&ctx.curr_frame) <= 0) {
   			break;
   		}
#endif
   }

   return true;
}
