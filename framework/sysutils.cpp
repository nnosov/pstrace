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

#include "logger/log.h"
#include "sysutils.h"

extern SC_LogBase* logger;

#define USEI_LIBUNWIND

#ifdef USEI_LIBUNWIND
#include <libunwind.h>
#endif

#include "common.h"

bool __pst_context::print(const char* fmt, ...)
{
	bool nret = true;

	va_list args;
	va_start(args, fmt);
	int size = sizeof(buff) - offset;
	int ret = vsnprintf(buff + offset, size, fmt, args);
	if(ret >= size || ret < 0) {
		nret = false;
	}
	offset += ret;
	va_end(args);

	return nret;
}

void __pst_context::log(SC_LogSeverity severity, const char* fmt, ...)
{
	uint32_t    str_len = 0;
	char        str[PATH_MAX]; str[0] = 0;
	va_list args;
	va_start(args, fmt);
	str_len += vsnprintf(str + str_len, sizeof(str) - str_len, fmt, args);
	va_end(args);

	logger->Log(severity, "%s", str);
}

// dwfl_addrsegment() possibly can be used to check address validity
// dwarf_getattrs() allows to enumerate all DIE attributes
// dwarf_getfuncs() allows to enumerate functions within CU

bool is_location_form(int form)
{
    if (form == DW_FORM_block1 || form == DW_FORM_block2 || form == DW_FORM_block4 || form == DW_FORM_block ||
    	form == DW_FORM_data4  || form == DW_FORM_data8  || form == DW_FORM_sec_offset) {
        return true;
    }
    return false;
}

char* GetExecutableName(char* path, uint32_t size)
{
    char link[PATH_MAX];
    snprintf(link, sizeof(link), "/proc/%d/exe", getpid());
    int nret = readlink(link, path, size);
    if (nret == -1) {
        return NULL;
    }

    // trailing zero
    path[nret] = 0;

    return path;
}

#include <inttypes.h>

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


uint32_t __pst_context::print_expr_block (Dwarf_Op *exprs, int len, char* buff, uint32_t buff_size, Dwarf_Attribute* attr)
{
	uint32_t offset = 0;
	for (int i = 0; i < len; i++) {
		//printf ("%s", (i + 1 < len ? ", " : ""));
		const dwarf_op_map* map = find_op_map(exprs[i].atom);
		if(map) {
			if(map->op_num >= DW_OP_breg0 && map->op_num <= DW_OP_breg16) {
				int32_t off = decode_sleb128((unsigned char*)&exprs[i].number);

				unw_word_t ptr = 0;
				unw_get_reg(&cursor, map->regno, &ptr);
				//ptr += off;

				offset += snprintf(buff + offset, buff_size - offset, "%s(*%s%s%d) reg_value: 0x%lX ", map->op_name, map->regname, off >=0 ? "+" : "", off, ptr);
			} else if(map->op_num >= DW_OP_reg0 && map->op_num <= DW_OP_reg16) {
				unw_word_t value = 0;
				unw_get_reg(&cursor, map->regno, &value);
				offset += snprintf(buff + offset, buff_size - offset, "%s(*%s) value: 0x%lX", map->op_name, map->regname, value);
			} else if(map->op_num == DW_OP_GNU_entry_value) {
				uint32_t value = decode_uleb128((unsigned char*)&exprs[i].number);
				offset += snprintf(buff + offset, buff_size - offset, "%s(%u, ", map->op_name, value);
				Dwarf_Attribute attr_mem;
				if(!dwarf_getlocation_attr(attr, exprs, &attr_mem)) {
					Dwarf_Op *expr;
					size_t exprlen;
					if (dwarf_getlocation(&attr_mem, &expr, &exprlen) == 0) {
						offset += print_expr_block (expr, exprlen, buff + offset, buff_size - offset, attr);
						offset += snprintf(buff + offset, buff_size - offset, ") ");
					} else {
						log(SEVERITY_ERROR, "Failed to get DW_OP_GNU_entry_value attr location");
					}
				} else {
					log(SEVERITY_ERROR, "Failed to get DW_OP_GNU_entry_value attr expression");
				}
			} else if(map->op_num == DW_OP_stack_value) {
				offset += snprintf(buff + offset, buff_size - offset, "%s", map->op_name);
			} else if(map->op_num == DW_OP_plus_uconst) {
				uint32_t value = decode_uleb128((unsigned char*)&exprs[i].number);
				offset += snprintf(buff + offset, buff_size - offset, "%s(+%u) ", map->op_name, value);
			} else if(map->op_num == DW_OP_bregx) {
				uint32_t regno = decode_uleb128((unsigned char*)&exprs[i].number);
				int32_t off = decode_sleb128((unsigned char*)&exprs[i].number2);

				unw_word_t ptr = 0;
				unw_get_reg(&cursor, map->regno, &ptr);
				//ptr += off;

				offset += snprintf(buff + offset, buff_size - offset, "%s(%s%s%d) reg_value = 0x%lX", map->op_name, unw_regname(regno), off >= 0 ? "+" : "", off, ptr);
			} else {
				offset += snprintf(buff + offset, buff_size - offset, "%s(0x%lX, 0x%lx) ", map->op_name, exprs[i].number, exprs[i].number2);
			}
		} else {
			offset += snprintf(buff + offset, buff_size - offset, "0x%hhX(0x%lX, 0x%lx) ", exprs[i].atom, exprs[i].number, exprs[i].number2);
		}
//		if(exprs[i].atom >= DW_OP_reg0 && exprs[i].atom <= DW_OP_bregx) {
//			print_framereg(exprs[i].atom);
//		}
	}

	return offset;
}

bool __pst_parameter::handle_type(Dwarf_Attribute* param, bool is_return)
{
	Dwarf_Attribute attr_mem;
	Dwarf_Attribute* attr;

	// get DIE of return type
	Dwarf_Die ret_die;

	if(!dwarf_formref_die(param, &ret_die)) {
		function->ctx->log(SEVERITY_ERROR, "Failed to get parameter DIE");
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
			function->ctx->log(SEVERITY_INFO, "base type '%s'(%lu)", dwarf_diename(&ret_die), size);
			types.push_back(dwarf_diename(&ret_die));
			break;
		}
		case DW_TAG_array_type:
			logger->Log(SEVERITY_INFO, "array type");
			types.push_back("[]");
			break;
		case DW_TAG_pointer_type:
			logger->Log(SEVERITY_INFO, "pointer type");
			types.push_back("*");
			break;
		case DW_TAG_enumeration_type:
			logger->Log(SEVERITY_INFO, "enumeration type");
			types.push_back("enum");
			break;
		case DW_TAG_const_type:
			logger->Log(SEVERITY_INFO, "constant type");
			types.push_back("const");
			break;
		case DW_TAG_subroutine_type:
			logger->Log(SEVERITY_INFO, "subroutine type");
			break;
		case DW_TAG_typedef:
			logger->Log(SEVERITY_INFO, "typedef '%s' type", dwarf_diename(&ret_die));
			types.push_back(dwarf_diename(&ret_die));
			break;
		default:
			logger->Log(SEVERITY_INFO, "Unknown 0x%X tag type", dwarf_tag(&ret_die));
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

	// Get reference to attribute type of the parameter/variable
	attr = dwarf_attr(result, DW_AT_type, &attr_mem);
	function->ctx->log(SEVERITY_INFO, "Handle '%s' %s", dwarf_diename(result), dwarf_tag(result) == DW_TAG_formal_parameter ? "parameter" : "variable");
	if(attr) {
		handle_type(attr);
	}

	// determine location of parameter in stack/heap or CPU registers
	attr = dwarf_attr(result, DW_AT_location, &attr_mem);
	if(attr) {
		if(dwarf_hasform(attr, DW_FORM_exprloc)) {
			Dwarf_Op *expr;
			size_t exprlen;
			if (dwarf_getlocation(attr, &expr, &exprlen) == 0) {
				char str[1024]; str[0] = 0;
				function->ctx->print_expr_block (expr, exprlen, str, sizeof(str), attr);
				function->ctx->log(SEVERITY_DEBUG, "Found DW_AT_location expression: %s", str);
//				if(expr[0].atom >= DW_OP_reg0 && expr[0].atom <= DW_OP_bregx) {
//					print_framereg(expr[0].atom);
//				}
			}
		} else if(dwarf_hasform(attr, DW_FORM_sec_offset)) {
			Dwarf_Addr base, start, end;
			ptrdiff_t off = 0;
			Dwarf_Op *expr;
			size_t exprlen;

			for(int i = 0; (off = dwarf_getlocations (attr, off, &base, &start, &end, &expr, &exprlen)) > 0; ++i) {
				char str[1024]; str[0] = 0;
				function->ctx->print_expr_block (expr, exprlen, str, sizeof(str), attr);
				function->ctx->log(SEVERITY_DEBUG, "[%d] low_offset: 0x%" PRIx64 ", high_offset: 0x%" PRIx64 " ==> %s", i, start, end, str);
			}

		} else {
			function->ctx->log(SEVERITY_WARNING, "Unknown attribute form = 0x%X, code = 0x%X, ", attr->form, attr->code);
		}
	}

	return true;
}

bool __pst_function::handle_dwarf(Dwarf_Die* d)
{
	die = d;

	Dwarf_Attribute attr_mem;
	Dwarf_Attribute* attr;

	// get offset to function in memory against base address where process executed
	dwarf_lowpc(d, &lowpc);
    Dwarf_Addr highpc;
    dwarf_highpc(d, &highpc);

    unw_proc_info_t info;
    unw_get_proc_info(&ctx->cursor, &info);

    logger->Log(SEVERITY_INFO, "Found function in debug info. name = %s(...), PC = 0x%lX, LOW_PC = 0x%lX, HIGH_PC = 0x%lX, offset from base address: 0x%lX, info start = 0x%lX, offset from info start: 0x%lX",
    		dwarf_diename(d), pc, lowpc, highpc, pc - ctx->base_addr, info.start_ip, info.start_ip - ctx->base_addr);

	// determine function's stack frame base
	attr = dwarf_attr(die, DW_AT_frame_base, &attr_mem);
	if(attr) {
		if(dwarf_hasform(attr, DW_FORM_exprloc)) {
			Dwarf_Op *expr;
			size_t exprlen;
			if (dwarf_getlocation (attr, &expr, &exprlen) == 0) {
				char str[1024]; str[0] = 0;
				ctx->print_expr_block (expr, exprlen, str, sizeof(str), attr);
				ctx->log(SEVERITY_DEBUG, "Found DW_AT_framebase expression: %s", str);
			} else {
				ctx->log(SEVERITY_WARNING, "Unknown attribute form = 0x%X, code = 0x%X", attr->form, attr->code);
			}
		}
	}

	// Get reference to return attribute type of the function
	// may be to use dwfl_module_return_value_location() instead
	pst_parameter ret_p(this); ret_p.is_return = true;
	attr = dwarf_attr(die, DW_AT_type, &attr_mem);
	if(attr) {
		ctx->log(SEVERITY_INFO, "Handle return parameter");
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
		pst_parameter param(this);

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

	Dwfl_Line *line = dwfl_getsrc(dwfl, addr);
	if(line != NULL) {
		int nline;
		Dwarf_Addr addr;
		const char* filename = dwfl_lineinfo (line, &addr, &nline, NULL, NULL, NULL);
		if(filename) {
			const char* str = strrchr(filename, '/');
			if(str && *str != 0) {
				str++;
			} else {
				str = filename;
			}
			ctx->print("%s:%d", str, nline);
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


bool __pst_context::get_frame()
{
    // get CFI (Call Frame Information) for current module
    // from handle_cfi()
    Dwarf_Addr mod_bias = 0;
    Dwarf_CFI* cfi = dwfl_module_eh_cfi(module, &mod_bias);
    //Dwarf_CFI* cfi = dwfl_module_dwarf_cfi(module, &mod_bias);
    if(!cfi) {
    	log(SEVERITY_INFO, "Cannot find CFI for module");
    	return false;
    }

    // get frame of CFI for address
    int result = dwarf_cfi_addrframe (cfi, addr - mod_bias, &frame);
    if (result == 0) {
    	// get frame information
    	log(SEVERITY_INFO, "Found CFI frame for module");
    	Dwarf_Addr start = addr;
    	Dwarf_Addr end = addr;
    	bool signalp;
    	int ra_regno = dwarf_frame_info (frame, &start, &end, &signalp);
    	if(ra_regno >= 0) {
    		start += mod_bias;
    		end += mod_bias;
    	}
    	log(SEVERITY_DEBUG, "Per '.eh_frame' info has %#" PRIx64 " => [%#" PRIx64 ", %#" PRIx64 "] in_signal = %s", addr, start, end, signalp ? "true" : "false");
    	if (ra_regno < 0)
    		log(SEVERITY_DEBUG, "return address register unavailable (%s)", dwarf_errmsg(0));
    	else {
    		reginfo info; info.regno = ra_regno;
    		dwfl_module_register_names(module, regname_callback, &info);
    		log(SEVERITY_DEBUG, "return address in reg%u%s ==> %s", ra_regno, signalp ? " (signal frame)" : "", info.regname);
    	}

    	// finally get CFA (Canonical Frame Address)
    	// Point cfa_ops to dummy to match print_detail expectations.
    	// (nops == 0 && cfa_ops != NULL => "undefined")
    	Dwarf_Op dummy;
    	Dwarf_Op *cfa_ops = &dummy;
    	size_t cfa_nops;
    	result = dwarf_frame_cfa(frame, &cfa_ops, &cfa_nops);
    	char str[1024]; str[0] = 0;
    	print_expr_block (cfa_ops, cfa_nops, str, sizeof(str));
    	log(SEVERITY_INFO, "Found CFA expression: %s", str);
    	//print_detail (result, cfa_ops, cfa_nops, mod_bias, "\tCFA ");
    }

    return true;
}

bool __pst_context::get_dwarf_function(pst_function& fun)
{
    Dwarf_Addr mod_cu = 0;
    // get CU(Compilation Unit) debug definition
    Dwarf_Die* cdie = dwfl_module_addrdie(module, addr, &mod_cu);
    //Dwarf_Die* cdie = dwfl_addrdie(dwfl, addr, &mod_bias);
    if(!cdie) {
    	logger->Log(SEVERITY_INFO, "Failed to find DWARF DIE for address %X", addr);
    	return false;
    }

    if(dwarf_tag(cdie) != DW_TAG_compile_unit) {
    	logger->Log(SEVERITY_DEBUG, "Skipping non-cu die. DWARF tag: 0x%X, name = %s", dwarf_tag(cdie), dwarf_diename(cdie));
    	return false;
    }

    Dwarf_Die result;
	if(dwarf_child(cdie, &result)) {
		logger->Log(SEVERITY_ERROR, "No child DIE found for CU %s", dwarf_diename(cdie));
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

void __pst_context::dwarf_print()
{

}

char *debuginfo_path = NULL;
Dwfl_Callbacks callbacks = {
        .find_elf           = dwfl_linux_proc_find_elf,
        .find_debuginfo     = dwfl_standard_find_debuginfo,
        .section_address    = dwfl_offline_section_address,
        .debuginfo_path     = &debuginfo_path,
};

#include <dlfcn.h>

bool __pst_context::unwind()
{
#ifdef REG_RIP // x86_64
    caller = (void *) hcontext->uc_mcontext.gregs[REG_RIP];
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
	base_addr = (uint64_t)info.dli_fbase;
	logger->Log(SEVERITY_INFO, "Process address information: dlopen handle: %p, base address: %p, object name: %s, symbol name: %s", handle, info.dli_fbase, info.dli_fname, info.dli_sname);

    int skipped = 0;
#ifndef USEI_LIBUNWIND
    int size = 0;
    void * array[50];
    size = backtrace(array, 50);
    for(int i = 0; i < size && array[i] != caller_address; ++i, ++skipped);
    print("Stack trace(caller = %p. Total stack frames: %d, skipped: %d):\n", caller_address, size, skipped);
#else
    unw_word_t  	pc;

    unw_getcontext(&context);
    unw_init_local(&cursor, &context);
    while (unw_step(&cursor) > 0) {
    	unw_get_reg(&cursor, UNW_REG_IP,  &pc);
    	if(pc == (uint64_t)caller) {
    		break;
    	} else {
    		++skipped;
    	}
    }
    print("Stack trace(caller = %p. Skipped stack frames: %d):\n", caller, skipped);
#endif

    dwfl = dwfl_begin(&callbacks);
    if(dwfl == NULL) {
        print("Failed to initialize libdw session for parse stack frames");
        return false;
    }

    if(dwfl_linux_proc_report(dwfl, getpid()) != 0 || dwfl_report_end(dwfl, NULL, NULL) !=0) {
    	print("Failed to parse debug section of executable");
    	return false;
    }

#ifndef USEI_LIBUNWIND
    for (int i = skipped, idx = 0; i < size ; ++i, ++idx) {
    	print("[%-2d] ", idx);
    	Dwarf_Addr addr = (uintptr_t)array[i];
#else
   for (int i = skipped, idx = 0; true ; ++i, ++idx) {
   		unw_get_reg(&cursor, UNW_REG_IP,  &pc);
   		addr = pc;
   		print("[%-2d] ", idx);
#endif
   		module = dwfl_addrmodule(dwfl, addr);
   		get_frame();
   		pst_function fun(this);
   		if(fun.unwind(dwfl, module, pc)) {
   			if(get_dwarf_function(fun)) {
   				functions.push_back(fun);
   			}
   		}
   		print("\n");
#ifdef USEI_LIBUNWIND
   		if(unw_step(&cursor) <= 0) {
   			break;
   		}
#endif
   }

//   for(int op = DW_OP_breg0; op <= DW_OP_breg16; op++ ) {
//	   printf("{0x%X, 0x%X, \"%s\"},\n", op, op - DW_OP_breg0, unw_regname(op - DW_OP_breg0));
//   }

   return true;
}
