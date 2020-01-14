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

#define USEI_LIBUNWIND

#ifdef USEI_LIBUNWIND
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
	char regname[1024];
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
    if(types.size()) {
        if(!is_return) {
            ctx->print("%s %s = 0x%lX", types[0].c_str(), name.c_str(), value);
        } else {
            ctx->print("%s", types[0].c_str());
        }
    } else {
        ctx->print("%s = 0x%lX", name.c_str(), value);
    }
    return true;
}

bool __pst_parameter::handle_type(Dwarf_Attribute* param, bool is_return)
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
			types.push_back(dwarf_diename(&ret_die));
			break;
		}
		case DW_TAG_array_type:
			ctx->log(SEVERITY_DEBUG, "array type");
			types.push_back("[]");
			break;
		case DW_TAG_structure_type:
			ctx->log(SEVERITY_DEBUG, "structure type");
			types.push_back("struct");
			break;
		case DW_TAG_union_type:
			ctx->log(SEVERITY_DEBUG, "union type");
			types.push_back("union");
			break;
		case DW_TAG_class_type:
			ctx->log(SEVERITY_DEBUG, "class type");
			types.push_back("class");
			break;
		case DW_TAG_pointer_type:
			ctx->log(SEVERITY_DEBUG, "pointer type");
			types.push_back("*");
			break;
		case DW_TAG_enumeration_type:
			ctx->log(SEVERITY_DEBUG, "enumeration type");
			types.push_back("enum");
			break;
		case DW_TAG_const_type:
			ctx->log(SEVERITY_DEBUG, "constant type");
			types.push_back("const");
			break;
		case DW_TAG_subroutine_type:
			ctx->log(SEVERITY_DEBUG, "subroutine type");
			break;
		case DW_TAG_typedef:
			ctx->log(SEVERITY_DEBUG, "typedef '%s' type", dwarf_diename(&ret_die));
			types.push_back(dwarf_diename(&ret_die));
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

	Dwarf_Addr pc;
	unw_get_reg(&ctx->cursor, UNW_REG_IP,  &pc);
	Dwarf_Addr offset = pc - ctx->base_addr;

	dwarf_stack stack(ctx);
	// determine location of parameter in stack/heap or CPU registers
	attr = dwarf_attr(result, DW_AT_location, &attr_mem);
	if(attr) {
		if(dwarf_hasform(attr, DW_FORM_exprloc)) {
			Dwarf_Op *expr;
			size_t exprlen;
			if(dwarf_getlocation(attr, &expr, &exprlen) == 0) {
				char str[1024]; str[0] = 0;
                ctx->print_expr_block (expr, exprlen, str, sizeof(str), attr);
                if(stack.calc_expression(expr, exprlen, attr)) {
                    if(stack.get_value(value)) {
                        ctx->log(SEVERITY_DEBUG, "DW_AT_location expression: \"%s\" ==> 0x%lX", str, value);
                    } else {
                        ctx->log(SEVERITY_ERROR, "Failed to get value of calculated DW_AT_location expression: %s", str);
                    }
                } else {
                    ctx->log(SEVERITY_ERROR, "Failed to calculate DW_AT_location expression: %s", str);
                }
			}
		} else if(dwarf_hasform(attr, DW_FORM_sec_offset)) {
			Dwarf_Addr base, start, end;
			ptrdiff_t off = 0;
			Dwarf_Op *expr;
			size_t exprlen;

			// handle list of possible locations of parameter
			for(int i = 0; (off = dwarf_getlocations (attr, off, &base, &start, &end, &expr, &exprlen)) > 0; ++i) {
			    if(offset >= start && offset <= end) {
			        // actual location, try to calculate Location expression and retrieve value of parameter
	                char str[1024]; str[0] = 0;
	                ctx->print_expr_block (expr, exprlen, str, sizeof(str), attr);
	                if(stack.calc_expression(expr, exprlen, attr)) {
	                    if(stack.get_value(value)) {
	                        ctx->log(SEVERITY_DEBUG, "Location list expression: [%d] (low_offset: 0x%" PRIx64 ", high_offset: 0x%" PRIx64"), \"%s\" ==> 0x%lX", i, start, end, str, value);
	                    } else {
                            ctx->log(SEVERITY_DEBUG, "Failed to get value of calculated Location list expression: [%d] (low_offset: 0x%" PRIx64 ", high_offset: 0x%" PRIx64 "), \"%s\" ==> 0x%lX",
                                    i, start, end, str, value);
	                    }
	                } else {
                        ctx->log(SEVERITY_DEBUG, "Failed to calculate Location list expression: [%d] (low_offset: 0x%" PRIx64 ", high_offset: 0x%" PRIx64 "), \"%s\" ==> 0x%lX",
                                i, start, end, str, value);
	                }

			    } else {
			        // Location skipped due to don't match current PC offset
			    }
			}

		} else {
			ctx->log(SEVERITY_WARNING, "Unknown attribute form = 0x%X, code = 0x%X, ", attr->form, attr->code);
		}
	}

	//  handle DW_AT_default_value to get information about default value for DW_TAG_formal_parameter type of function

	return true;
}

bool __pst_function::print_dwarf()
{
    //std::vector<std::string>::const_iterator param;
    //for (param = params.begin(); param != params.end(); ++param) {

    ctx->print("%s:%u: ", file.c_str(), line);
    bool first = true; bool start_variable = false;
    for(auto param : params) {
        if(param.is_return) {
            // print return value, function name and start list of parameters
            param.print_dwarf();
            ctx->print(" %s(", name.c_str());
            continue;
        }

        if(param.is_variable) {
            if(!start_variable) {
                ctx->print(")\n");
                ctx->print("{\n");
                start_variable = true;
            }
            if(param.line) {
                ctx->print("%04u:   ", param.line);
            } else {
                ctx->print("        ");
            }
            param.print_dwarf();
            ctx->print(";\n");
        } else {
            if(first) {
                first = false;
            } else {
                ctx->print(", ");
            }
            param.print_dwarf();
        }
    }
    if(!start_variable) {
        ctx->print(");\n");
    } else {
        ctx->print("}\n");
    }

    return true;
}

bool __pst_function::handle_dwarf(Dwarf_Die* d)
{
	die = d;

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
    unw_get_proc_info(&ctx->cursor, &info);
	unw_word_t sp;
	unw_get_reg(&ctx->cursor, UNW_REG_SP, &sp);

    ctx->log(SEVERITY_DEBUG, "Found function in debug info. name = %s(...), PC = 0x%lX, LOW_PC = 0x%lX, HIGH_PC = 0x%lX, offset from base address: 0x%lX, BASE_PC = 0x%lX, offset from start of function: 0x%lX, stack address: 0x%lX",
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
				        ctx->log(SEVERITY_DEBUG, "DW_AT_framebase expression: \"%s\"==> 0x%lX", str, value);
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
	pst_parameter ret_p(ctx); ret_p.is_return = true;
	attr = dwarf_attr(die, DW_AT_type, &attr_mem);
	if(attr) {
		ctx->log(SEVERITY_DEBUG, "Handle return parameter");
		if(ret_p.handle_type(attr, true)) {
			params.push_back(ret_p);
		}
	} else {
		ret_p.types.push_back("void");
		params.push_back(ret_p);
		ctx->log(SEVERITY_DEBUG, "return attr name = 'void(0)'");
	}

	Dwarf_Die result;
	if(dwarf_child(die, &result) != 0)
		return false;

	// went through parameters and local variables of the function
	do {
		pst_parameter param(ctx);

		switch (dwarf_tag(&result)) {
		case DW_TAG_formal_parameter:
		case DW_TAG_variable:
			if(param.handle_dwarf(&result)) {
				params.push_back(param);
			}
			break;
			//				case DW_TAG_inlined_subroutine:
			//					/* Recurse further down */
			//					HandleFunction(&result, dwarf_diename(&result));
			//					break;
		default:
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
    	ctx.log(SEVERITY_INFO, "Cannot find CFI for module");
    	return false;
    }

    // get frame of CFI for address
    int result = dwarf_cfi_addrframe (cfi, addr - mod_bias, &frame);
    if (result == 0) {
    	// get frame information
    	ctx.log(SEVERITY_INFO, "Found CFI frame for module");
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
    	ctx.log(SEVERITY_INFO, "Found CFA expression: %s", str);
    	//print_detail (result, cfa_ops, cfa_nops, mod_bias, "\tCFA ");
    }

    return true;
}

bool __pst_handler::get_dwarf_function(pst_function& fun)
{
    Dwarf_Addr mod_cu = 0;
    // get CU(Compilation Unit) debug definition
    Dwarf_Die* cdie = dwfl_module_addrdie(module, addr, &mod_cu);
    //Dwarf_Die* cdie = dwfl_addrdie(dwfl, addr, &mod_bias);
    if(!cdie) {
    	ctx.log(SEVERITY_INFO, "Failed to find DWARF DIE for address %X", addr);
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
			if(!strcmp(fun.name.c_str(), dwarf_diename(&result))) {
				return fun.handle_dwarf(&result);
			}
		}
	} while(dwarf_siblingof(&result, &result) == 0);

	return nret;
}

void __pst_handler::print_dwarf()
{
    ctx.offset = 0;
    ctx.print("DWARF-based stack trace information:\n");
    for(auto function : functions) {
        function.print_dwarf();
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

    handle = dlopen(NULL, RTLD_NOW);
	Dl_info info;
	dladdr(caller, &info);
	ctx.base_addr = (uint64_t)info.dli_fbase;
	ctx.log(SEVERITY_INFO, "Process address information: base address: %p, object name: %s", info.dli_fbase, info.dli_fname);

    int skipped = 0;
#ifndef USEI_LIBUNWIND
    int size = 0;
    void * array[50];
    size = backtrace(array, 50);
    for(int i = 0; i < size && array[i] != caller_address; ++i, ++skipped);
    print("Stack trace(caller = %p. Total stack frames: %d, skipped: %d):\n", caller_address, size, skipped);
#else
    unw_word_t  	pc;

    unw_getcontext(&ctx.context);
    unw_init_local(&ctx.cursor, &ctx.context);
    while (unw_step(&ctx.cursor) > 0) {
    	unw_get_reg(&ctx.cursor, UNW_REG_IP,  &pc);
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

#ifndef USEI_LIBUNWIND
    for (int i = skipped, idx = 0; i < size ; ++i, ++idx) {
    	print("[%-2d] ", idx);
    	Dwarf_Addr addr = (uintptr_t)array[i];
#else
   for (int i = skipped, idx = 0; true ; ++i, ++idx) {
   		unw_get_reg(&ctx.cursor, UNW_REG_IP,  &pc);
   		addr = pc;
   		ctx.print("[%-2d] ", idx);
#endif
   		module = dwfl_addrmodule(dwfl, addr);
   		//get_frame();
   		pst_function fun(&ctx);
   		if(fun.unwind(dwfl, module, pc)) {
   			if(get_dwarf_function(fun)) {
   				functions.push_back(fun);
   			}
   		}
   		ctx.print("\n");
#ifdef USEI_LIBUNWIND
   		if(unw_step(&ctx.cursor) <= 0) {
   			break;
   		}
#endif
   }

//   for(int op = DW_OP_breg0; op <= DW_OP_breg16; op++ ) {
//	   printf("{0x%X, 0x%X, \"%s\"},\n", op, op - DW_OP_breg0, unw_regname(op - DW_OP_breg0));
//   }

   return true;
}
