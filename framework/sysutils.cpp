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

char __stack_trace[8192];


Dwarf_Frame* frame;
Dwfl_Module* module;

// dwfl_addrsegment() possibly can be used to check address validity
// dwarf_getattrs() allows to enumerate all DIE attributes
// dwarf_getfuncs() allows to enumerate functions within CU

static void print_detail (int result, const Dwarf_Op *ops, size_t nops, Dwarf_Addr bias, const char* prefix);

bool is_location_form(int form)
{
    if (form == DW_FORM_block1 ||
        form == DW_FORM_block2 ||
        form == DW_FORM_block4 ||
        form == DW_FORM_block ||
        form == DW_FORM_data4 ||
        form == DW_FORM_data8 ||
        form == DW_FORM_sec_offset) {
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

void HandleType(Dwarf_Attribute* param, bool is_return = false)
{
	Dwarf_Attribute attr_mem;
	Dwarf_Attribute* attr;

	// get DIE of return type
	Dwarf_Die ret_die;

	if(dwarf_formref_die(param, &ret_die)) {
		switch (dwarf_tag(&ret_die)) {
			case DW_TAG_base_type: {
				// get Size attribute and it's value
				Dwarf_Word size = 0;
				attr = dwarf_attr(&ret_die, DW_AT_byte_size, &attr_mem);
				if(attr) {
					dwarf_formudata(attr, &size);
				}
				logger->Log(SEVERITY_INFO, "base type '%s'(%lu)", dwarf_diename(&ret_die), size);
				break;
			}
			case DW_TAG_array_type:
				logger->Log(SEVERITY_INFO, "array type");
				break;
			case DW_TAG_pointer_type:
				logger->Log(SEVERITY_INFO, "pointer type");
				break;
			case DW_TAG_enumeration_type:
				logger->Log(SEVERITY_INFO, "enumeration type");
				break;
			case DW_TAG_const_type:
				logger->Log(SEVERITY_INFO, "constant type");
				break;
			case DW_TAG_subroutine_type:
				logger->Log(SEVERITY_INFO, "subroutine type");
				break;
			case DW_TAG_typedef:
				logger->Log(SEVERITY_INFO, "typedef '%s' type", dwarf_diename(&ret_die));
				break;
			default:
				logger->Log(SEVERITY_INFO, "Unknown 0x%X tag type", dwarf_tag(&ret_die));
				break;
		}
		attr = dwarf_attr(&ret_die, DW_AT_type, &attr_mem);
		if(attr) {
			HandleType(attr);
		}
	}
}

void
print_expr_block (Dwarf_Attribute *attr, Dwarf_Op *exprs, int len,
		  Dwarf_Addr addr, int depth)
{
  printf ("{");
  for (int i = 0; i < len; i++)
    {
      printf ("%s", (i + 1 < len ? ", " : ""));
    }
  printf ("}");
}

void HandleParameter(Dwarf_Die* result)
{
	Dwarf_Attribute attr_mem;
	Dwarf_Attribute* attr;

	// Get reference to attribute type of the parameter/variable
	attr = dwarf_attr(result, DW_AT_type, &attr_mem);
	logger->Log(SEVERITY_INFO, "Handle '%s' %s", dwarf_diename(result), dwarf_tag(result) == DW_TAG_formal_parameter ? "parameter" : "variable");
	if(attr) {
		HandleType(attr);
	}

	// determine location of parameter in stack/heap or CPU registers
	attr = dwarf_attr(result, DW_AT_location, &attr_mem);
	if(attr) {
		logger->Log(SEVERITY_DEBUG, "parameter location attribute form = 0x%X, code = 0x%X", attr->form, attr->code);
		if(dwarf_hasform(attr, DW_FORM_exprloc)) {
			Dwarf_Op *expr;
			size_t exprlen;
			if (dwarf_getlocation(attr, &expr, &exprlen) == 0) {
				logger->Log(SEVERITY_DEBUG, "Found DW_AT_location expression");
				print_detail(0, expr, exprlen, 0, "\tLocation ");
				if(expr[0].atom >= DW_OP_reg0 && expr[0].atom <= DW_OP_bregx) {
					Dwarf_Op ops_mem[3];
					Dwarf_Op* ops;
					size_t nops;
					int ret = dwarf_frame_register(frame, expr[0].atom, ops_mem, &ops, &nops);
					if(ret != 0) {
						logger->Log(SEVERITY_DEBUG, "Failed to get CFI expression for register 0x%hhX", expr[0].atom);
						return;
					}
					if(nops == 0 && ops == ops_mem) {
						logger->Log(SEVERITY_DEBUG, "CFI expression for register 0x%hhX is UNDEFINED", expr[0].atom);
						return;
					}
					if(nops == 0 && ops == NULL) {
						logger->Log(SEVERITY_DEBUG, "CFI expression for register 0x%hhX is SAME VALUE", expr[0].atom);
						return;
					}
					logger->Log(SEVERITY_DEBUG, "Found CFI expression for register 0x%hhX, number of expressions: %d", expr[0].atom, nops);
					for(size_t i = 0; i < nops; ++i) {
						logger->Log(SEVERITY_DEBUG, "\t[%d] operation: 0x%hhX, operand1: 0x%lX, operand2: 0x%lx, offset: 0x%lX", i, ops[i].atom, ops[i].number, ops[i].number2, ops[i].offset);
					}
				}
			}
		} else if(dwarf_hasform(attr, DW_FORM_sec_offset)) {
			Dwarf_Addr base, start, end;
			ptrdiff_t off = 0;
			Dwarf_Op *fb_expr;
			size_t fb_exprlen;
			while ((off = dwarf_getlocations (attr, off, &base, &start, &end, &fb_expr, &fb_exprlen)) > 0) {
				printf ("      (%" PRIx64 ",%" PRIx64 ") ", start, end);
				print_expr_block (attr, fb_expr, fb_exprlen, start, 0);
				printf ("\n");
			}


			Dwarf_Word value;

			int ret = dwarf_formudata(attr, &value);
			if(ret >= 0) {
				logger->Log(SEVERITY_DEBUG, "Found DW_AT_location offset = 0x%lX", value);
			}
		} else {
			logger->Log(SEVERITY_WARNING, "Unknown attribute form = 0x%X, code = 0x%X", attr->form, attr->code);
		}

	}
}

void HandleFunction(Dwarf_Die* func, const char* fname)
{
	if(!strcmp(fname, dwarf_diename(func))) {
		logger->Log(SEVERITY_INFO, "Found function in debug info. DWARF tag: 0x%X, name = %s(...)", dwarf_tag(func), dwarf_diename(func));

		Dwarf_Attribute attr_mem;
		Dwarf_Attribute* attr;

		// determine function's stack frame base
		attr = dwarf_attr(func, DW_AT_frame_base, &attr_mem);
		if(attr) {
			if(dwarf_hasform(attr, DW_FORM_exprloc)) {
				Dwarf_Op *expr;
				size_t exprlen;
				if (dwarf_getlocation (attr, &expr, &exprlen) == 0) {
					logger->Log(SEVERITY_DEBUG, "Found DW_AT_framebase expression");
					print_detail(0, expr, exprlen, 0, "\tLocation ");
				} else {
					logger->Log(SEVERITY_WARNING, "Unknown attribute form = 0x%X, code = 0x%X", attr->form, attr->code);
				}
			}
		}

		// Get reference to return attribute type of the function
		// may be to use dwfl_module_return_value_location() instead
		attr = dwarf_attr(func, DW_AT_type, &attr_mem);
		if(attr) {
			logger->Log(SEVERITY_INFO, "Handle return parameter");
			HandleType(attr, true);
		} else {
			logger->Log(SEVERITY_DEBUG, "return attr name = 'void(0)'");
		}

		Dwarf_Die result;
		if (dwarf_child(func, &result) != 0)
		    return;

		// went through parameters and local variables of the function
		do {
			switch (dwarf_tag(&result)) {
				case DW_TAG_formal_parameter:
				case DW_TAG_variable:
					HandleParameter(&result);
					break;
//				case DW_TAG_inlined_subroutine:
//					/* Recurse further down */
//					HandleFunction(&result, dwarf_diename(&result));
//					break;
				default:
					break;
			}
		} while(dwarf_siblingof(&result, &result) == 0);
	}
	return;
}

static void print_detail (int result, const Dwarf_Op *ops, size_t nops, Dwarf_Addr bias, const char* prefix)
{
	printf("\t%s ", prefix);
	if (result < 0) {
		printf("indeterminate (%s)\n", dwarf_errmsg (-1));
	} else if (nops == 0) {
		printf("%s\n", ops == NULL ? "same_value" : "undefined");
	} else {
		printf("%s expression:", result == 0 ? "location" : "value");
		for (size_t i = 0; i < nops; ++i) {
			printf (" 0x%X (offset: 0x%lX)", ops[i].atom, ops[i].offset);
			if (ops[i].number2 == 0) {
				if (ops[i].atom == DW_OP_addr) {
					printf ("(%#" PRIx64 ")", ops[i].number + bias);
				} else if (ops[i].number != 0) {
					printf ("(%" PRIx64 ")", ops[i].number);
				}
			}
			else {
				printf ("(%" PRIx64 ",%" PRIx64 ")", ops[i].number, ops[i].number2);
			}
		}
		puts("");
	}
}

void HandleCompilationUnit(Dwfl_Module* module, Dwarf_Addr addr, const char* fname)
{
    Dwarf_Addr mod_bias = 0;
    // get function debug definition
    Dwarf_Die* cdie = dwfl_module_addrdie(module, addr, &mod_bias);
    logger->Log(SEVERITY_DEBUG, "Function bias in module is 0x%lX", mod_bias);
    //Dwarf_Die* cdie = dwfl_addrdie(dwfl, addr, &mod_bias);
    if(!cdie) {
    	logger->Log(SEVERITY_INFO, "Failed to find DWARF DIE for address %X", addr);
    	return;
    }

    // get CFI (Call Frame Information) for current module
    // from handle_cfi()
    Dwarf_CFI* cfi = dwfl_module_eh_cfi(module, &mod_bias);
    if(cfi) {
    	// get frame of CFI for address
    	logger->Log(SEVERITY_INFO, "Found CFI for module %s", dwarf_diename(cdie));
    	int result = dwarf_cfi_addrframe (cfi, addr - mod_bias, &frame);
    	if (result == 0) {
    		// get frame information
        	logger->Log(SEVERITY_INFO, "Found CFI frame for module %s", dwarf_diename(cdie));
        	Dwarf_Addr start = addr;
        	Dwarf_Addr end = addr;
        	bool signalp;
        	int ra_regno = dwarf_frame_info (frame, &start, &end, &signalp);
        	if (ra_regno >= 0)
        	{
        		start += mod_bias;
        		end += mod_bias;
        	}
        	logger->Log(SEVERITY_DEBUG, "Per '.eh_frame' info has %#" PRIx64 " => [%#" PRIx64 ", %#" PRIx64 "] in_signal = %s", addr, start, end, signalp ? "true" : "false");
        	  if (ra_regno < 0)
        	    logger->Log(SEVERITY_DEBUG, "return address register unavailable (%s)", dwarf_errmsg (0));
        	  else {
        		  reginfo info; info.regno = ra_regno;
        		  dwfl_module_register_names(module, regname_callback, &info);
        		  logger->Log(SEVERITY_DEBUG, "return address in reg%u%s ==> %s", ra_regno, signalp ? " (signal frame)" : "", info.regname);
        	  }

        	  // finally get CFA (Canonical Frame Address)
        	  // Point cfa_ops to dummy to match print_detail expectations.
        	  // (nops == 0 && cfa_ops != NULL => "undefined")
        	  Dwarf_Op dummy;
        	  Dwarf_Op *cfa_ops = &dummy;
        	  size_t cfa_nops;
        	  result = dwarf_frame_cfa(frame, &cfa_ops, &cfa_nops);

        	  print_detail (result, cfa_ops, cfa_nops, mod_bias, "\tCFA ");
    	}

    }

    if(dwarf_tag(cdie) != DW_TAG_compile_unit) {
    	logger->Log(SEVERITY_DEBUG, "Skipping non-cu die. DWARF tag: 0x%X, name = %s", dwarf_tag(cdie), dwarf_diename(cdie));
    	return;
    }

	logger->Log(SEVERITY_DEBUG, "Enumerating Compilation Unit '%s' to lookup for function %s()", dwarf_diename(cdie), fname);
	Dwarf_Die result;
	if(dwarf_child(cdie, &result)) {
		logger->Log(SEVERITY_INFO, "No child DIE found for CU");
		return;
	}

	do {
		switch (dwarf_tag(&result)) {
			case DW_TAG_subprogram:
			case DW_TAG_entry_point:
			case DW_TAG_inlined_subroutine:
				HandleFunction(&result, fname);
				break;
			default:
				//logger->Log(SEVERITY_INFO, "Unknown tag 0x%X for die '%s'", dwarf_tag(&result), dwarf_diename(&result));
				break;
		}
	} while(dwarf_siblingof(&result, &result) == 0);
}

const char* LibdwTraceCallStack(ucontext_t* uctx)
{
    void * caller_address = 0;
#ifdef REG_RIP // x86_64
    caller_address = (void *) uctx->uc_mcontext.gregs[REG_RIP];
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
    void * array[50];
    int size = backtrace(array, 50);
    int skipped = 0;
    for(int i = 0; i < size && array[i] != caller_address; ++i, ++skipped);
    __stack_trace[0] = 0; uint32_t offset = 0;
    offset += snprintf(__stack_trace + offset, sizeof(__stack_trace) - offset, "Stack trace(caller = %p. Total stack frames: %d, skipped: %d):\n",
            caller_address, size, skipped);

    char *debuginfo_path = NULL;
    Dwfl_Callbacks callbacks = {
            .find_elf           = dwfl_linux_proc_find_elf,
            .find_debuginfo     = dwfl_standard_find_debuginfo,
            .section_address    = dwfl_offline_section_address,
            .debuginfo_path     = &debuginfo_path,
    };

    Dwfl* dwfl = dwfl_begin(&callbacks);
    if(dwfl == NULL) {
        offset += snprintf(__stack_trace + offset, sizeof(__stack_trace) - offset, "Failed to initialize libdw session for parse stack frames");
        return __stack_trace;
    }

    if(dwfl_linux_proc_report(dwfl, getpid()) != 0 || dwfl_report_end(dwfl, NULL, NULL) !=0) {
        offset += snprintf(__stack_trace + offset, sizeof(__stack_trace) - offset, "Failed to parse debug section of executable");
        return __stack_trace;
    }

    for (int i = skipped, idx = 0; i < size ; ++i, ++idx) {
        offset += snprintf(__stack_trace + offset, sizeof(__stack_trace) - offset, "[%-2d] ", idx);

        Dwarf_Addr addr = (uintptr_t)array[i];
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
                offset += snprintf(__stack_trace + offset, sizeof(__stack_trace) - offset, "%s:%d", str, nline);
            } else {
                offset += snprintf(__stack_trace + offset, sizeof(__stack_trace) - offset, "%p", array[i]);
            }
        } else {
            offset += snprintf(__stack_trace + offset, sizeof(__stack_trace) - offset, "%p", array[i]);
        }

        module = dwfl_addrmodule(dwfl, addr);
        const char* addrname = dwfl_module_addrname(module, addr);
        char* demangle_name = NULL;
        if(addrname) {
            int status;
            demangle_name = abi::__cxa_demangle(addrname, NULL, NULL, &status);
            char* function_name = NULL;
            if(asprintf(&function_name, "%s%s", demangle_name ? demangle_name : addrname, demangle_name ? "" : "()") == -1) {
            	logger->Log(SEVERITY_ERROR, "Failed to allocate memory");
            	return __stack_trace;
            }
            offset += snprintf(__stack_trace + offset, sizeof(__stack_trace) - offset, " --> %s", function_name);

            char* str = strchr(function_name, '(');
            if(str) {
            	*str = 0;
            }
            HandleCompilationUnit(module, addr, function_name);
            free(function_name);
        }

        if(demangle_name) {
            free(demangle_name);
        }

        offset += snprintf(__stack_trace + offset, sizeof(__stack_trace) - offset, "\n");
    }

    return __stack_trace;
}
