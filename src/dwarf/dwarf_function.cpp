/*
 * dwarf_function.cpp
 *
 *  Created on: Feb 1, 2020
 *      Author: nnosov
 */

#include "../src/dwarf/dwarf_function.h"

#include <dwarf.h>
#include <stdlib.h>
#include <cxxabi.h>

#include "../src/dwarf/dwarf_stack.h"
#include "../src/dwarf/dwarf_utils.h"

// -----------------------------------------------------------------------------------
// pst_function
// -----------------------------------------------------------------------------------
bool get_frame(pst_function* fn)
{
    // get CFI (Call Frame Information) for current module
    // from handle_cfi()
    Dwarf_Addr mod_bias = 0;
    Dwarf_CFI* cfi = dwfl_module_eh_cfi(fn->ctx->module, &mod_bias); // rty .eh_cfi first
    if(!cfi) { // then try .debug_fame second
        cfi = dwfl_module_dwarf_cfi(fn->ctx->module, &mod_bias);
    }
    if(!cfi) {
        pst_log(SEVERITY_ERROR, "Cannot find CFI for module");
        return false;
    }

    // get frame of CFI for address
    int result = dwarf_cfi_addrframe (cfi, fn->pc - mod_bias, &fn->frame);
    if (result != 0) {
        pst_log(SEVERITY_ERROR, "Failed to find CFI frame for module");
        return false;
    }

    // setup context to match frame
    fn->ctx->frame = fn->frame;

    // get return register and PC range for function
    Dwarf_Addr start = fn->pc;
    Dwarf_Addr end = fn->pc;
    bool signalp;
    int ra_regno = dwarf_frame_info (fn->frame, &start, &end, &signalp);
    if(ra_regno >= 0) {
        start += mod_bias;
        end += mod_bias;
        reginfo info; info.regno = ra_regno;
        dwfl_module_register_names(fn->ctx->module, regname_callback, &info);
        pst_log(SEVERITY_INFO, "Function %s(...): '.eh/debug frame' info: PC range:  => [%#" PRIx64 ", %#" PRIx64 "], return register: %s, in_signal = %s",
                fn->name, start, end, info.regname, signalp ? "true" : "false");
    } else {
        pst_log(SEVERITY_WARNING, "Return address register info unavailable (%s)", dwarf_errmsg(0));
    }

    // finally get CFA (Canonical Frame Address)
    // Point cfa_ops to dummy to match print_detail expectations.
    // (nops == 0 && cfa_ops != NULL => "undefined")
    Dwarf_Op dummy;
    Dwarf_Op *cfa_ops = &dummy;
    size_t cfa_nops;
    if(dwarf_frame_cfa(fn->frame, &cfa_ops, &cfa_nops)) {
        pst_log(SEVERITY_ERROR, "Failed to get CFA for frame");
        return false;
    }

    fn->ctx->print_expr(fn->ctx, cfa_ops, cfa_nops, NULL);

    pst_decl(pst_dwarf_stack, stack, fn->ctx);
    if(pst_dwarf_stack_calc(&stack, cfa_ops, cfa_nops, NULL, NULL) && pst_dwarf_stack_get_value(&stack, &fn->cfa)) {
        pst_log(SEVERITY_INFO, "Function %s(...): CFA expression: %s ==> %#lX", fn->name, fn->ctx->buff, fn->cfa);

        // setup context to match CFA for frame
        fn->ctx->cfa = fn->cfa;
    } else {
        pst_log(SEVERITY_ERROR, "Failed to calculate CFA expression");
    }

    pst_dwarf_stack_fini(&stack);

    return true;
}

pst_parameter* add_param(pst_function* fn)
{
    pst_new(pst_parameter, p, fn->ctx);
    list_add_bottom(&fn->params, &p->node);

    return p;
}

void del_param(pst_parameter* p)
{
    list_del(&p->node);
    pst_free(p);
}

void clear(pst_function* fn)
{
    pst_parameter*  param = NULL;
    struct list_node  *pos, *tn;
    list_for_each_entry_safe(param, pos, tn, &fn->params, node) {
        list_del(&param->node);
        pst_parameter_fini(param);
    }

    pst_call_site_storage_fini(&fn->call_sites);
}

pst_parameter* next_param(pst_function* fn, pst_parameter* p)
{
    struct list_node* n = (p == NULL) ? list_first(&fn->params) : list_next(&p->node);

    pst_parameter* ret = NULL;
    if(n) {
        ret = list_entry(n, pst_parameter, node);
    }

    return ret;
}

bool handle_lexical_block(pst_function* fn, Dwarf_Die* result)
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
                        pst_log(SEVERITY_DEBUG, "Abstract origin: %s('%s')", dwarf_diename(&origin), dwarf_diename (&child));
                        break;
                    // Also handle  DW_TAG_unspecified_parameters (unknown number of arguments i.e. fun(arg1, ...);
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
    pst_log(SEVERITY_DEBUG, "Lexical block with name '%s', tag 0x%X and origin '%s' found. lowpc = 0x%lX, highpc = 0x%lX", die_name, dwarf_tag (result), origin_name, lowpc, highpc);
    Dwarf_Die child;
    if(dwarf_child (result, &child) == 0) {
        do {
            switch (dwarf_tag (&child)) {
                case DW_TAG_lexical_block:
                    handle_lexical_block(fn, &child);
                    break;
                case DW_TAG_variable: {
                    pst_parameter* param = add_param(fn);
                    if(!pst_parameter_handle_dwarf(param, &child, fn)) {
                        del_param(param);
                    }
                    break;
                }
                case DW_TAG_GNU_call_site:
                    pst_call_site_storage_handle_dwarf(&fn->call_sites, &child, fn);
                    break;
                case DW_TAG_inlined_subroutine:
                    pst_log(SEVERITY_DEBUG, "Skipping Lexical block tag 'DW_TAG_inlined_subroutine'");
                    break;
                default:
                    pst_log(SEVERITY_DEBUG, "Unknown Lexical block tag 0x%X", dwarf_tag(&child));
                    break;
            }
        }while (dwarf_siblingof (&child, &child) == 0);
    }

    return true;
}

bool pst_function_print_dwarf(pst_function* fn)
{
    char* at = NULL;
    if(!asprintf(&at, " at %s:%d, %p", fn->file, fn->line, (void*)fn->pc)) {
        return false;
    }
    if(at[4] == ':' && at[5] == '-') {
        free(at);
        if(!asprintf(&at, " at %p", (void*)fn->pc)) {
            return false;
        }
    }
    //ctx->print(ctx, "%s:%d: ", file.c_str(), line);

    // handle return parameter and be safe if function haven't parameters (for example, dwar info for function is absent)
    pst_parameter* param = next_param(fn, NULL);
    if(param && param->is_return) {
        // print return value type, function name and start list of parameters
        pst_parameter_print_dwarf(param);
        fn->ctx->print(fn->ctx, " %s(", fn->name);
        param = next_param(fn, param);
    } else {
        fn->ctx->print(fn->ctx, "%s(", fn->name);
    }

    bool first = true; bool start_variable = false;
    for(; param; param = next_param(fn, param)) {
        if(param->is_return) {
            // print return value type, function name and start list of parameters
            pst_parameter_print_dwarf(param);
            fn->ctx->print(fn->ctx, " %s(", fn->name);
            continue;
        }

        if(param->is_variable) {
            if(!start_variable) {
                fn->ctx->print(fn->ctx, ")%s\n", at);
                fn->ctx->print(fn->ctx, "{\n");
                start_variable = true;
            }
            if(param->line) {
                fn->ctx->print(fn->ctx, "%04u:   ", param->line);
            } else {
                fn->ctx->print(fn->ctx, "        ");
            }
            pst_parameter_print_dwarf(param);
            fn->ctx->print(fn->ctx, ";\n");
        } else {
            if(first) {
                first = false;
            } else {
                fn->ctx->print(fn->ctx, ", ");
            }
            pst_parameter_print_dwarf(param);
        }
    }

    if(!start_variable) {
        fn->ctx->print(fn->ctx, ");%s\n", at);
    } else {
        fn->ctx->print(fn->ctx, "}\n");
    }

    free(at);
    return true;
}

bool pst_function_handle_dwarf(pst_function * fn, Dwarf_Die* d)
{
    fn->die = d;
    get_frame(fn);

    Dwarf_Attribute attr_mem;
    Dwarf_Attribute* attr;

    // get list of offsets from process base address of continuous memory ranges where function's code resides
//  if(dwarf_haspc(d, pc)) {
    dwarf_lowpc(d, &fn->lowpc);
    dwarf_highpc(d, &fn->highpc);
//  } else {
//      pst_log(SEVERITY_ERROR, "Function's '%s' DIE hasn't definitions of memory offsets of function's code", dwarf_diename(d));
//      return false;
//  }

    unw_proc_info_t info;
    unw_get_proc_info(&fn->cursor, &info);
    fn->ctx->clean_print(fn->ctx);

    pst_log(SEVERITY_INFO, "Function %s(...): LOW_PC = %#lX, HIGH_PC = %#lX, offset from base address: 0x%lX, START_PC = 0x%lX, offset from start of function: 0x%lX",
            dwarf_diename(d), fn->lowpc, fn->highpc, fn->pc - fn->ctx->base_addr, info.start_ip, info.start_ip - fn->ctx->base_addr);
    fn->ctx->print_registers(fn->ctx, 0x0, 0x10);
    pst_log(SEVERITY_INFO, "Function %s(...): CFA: %#lX %s", dwarf_diename(d), fn->parent ? fn->parent->sp : 0, fn->ctx->buff);
    pst_log(SEVERITY_INFO, "Function %s(...): %s", dwarf_diename(d), fn->ctx->buff);

    // determine function's stack frame base
    attr = dwarf_attr(fn->die, DW_AT_frame_base, &attr_mem);
    if(attr) {
        if(dwarf_hasform(attr, DW_FORM_exprloc)) {
            Dwarf_Op *expr;
            size_t exprlen;
            if (dwarf_getlocation (attr, &expr, &exprlen) == 0) {
                fn->ctx->print_expr(fn->ctx, expr, exprlen, attr);
                pst_decl(pst_dwarf_stack, stack, fn->ctx);
                if(pst_dwarf_stack_calc(&stack, expr, exprlen, attr, fn)) {
                    uint64_t value;
                    if(pst_dwarf_stack_get_value(&stack, &value)) {
                        pst_log(SEVERITY_DEBUG, "DW_AT_framebase expression: \"%s\" ==> 0x%lX", fn->ctx->buff, value);
                    } else {
                        pst_log(SEVERITY_ERROR, "Failed to get value of calculated DW_AT_framebase expression: %s", fn->ctx->buff);
                    }
                } else {
                    pst_log(SEVERITY_ERROR, "Failed to calculate DW_AT_framebase expression: %s", fn->ctx->buff);
                }
                pst_dwarf_stack_fini(&stack);
            } else {
                pst_log(SEVERITY_WARNING, "Unknown attribute form = 0x%X, code = 0x%X", attr->form, attr->code);
            }
        }
    }

    // Get reference to return attribute type of the function
    // may be to use dwfl_module_return_value_location() instead
    pst_parameter* ret_p = add_param(fn); ret_p->is_return = true;
    attr = dwarf_attr(fn->die, DW_AT_type, &attr_mem);
    if(attr) {
        if(!pst_parameter_handle_type(ret_p, attr)) {
            pst_log(SEVERITY_ERROR, "Failed to handle return parameter type for function %s(...)", fn->name);
            del_param(ret_p);
        }
    } else {
        pst_parameter_add_type(ret_p, "void", 0);
    }

    // handle and save additionally these attributes:
    // 1. string of DW_AT_linkage_name (mangled name of program if any)
    //      A debugging information entry may have a DW_AT_linkage_name attribute
    //      whose value is a null-terminated string containing the object file linkage name
    //      associated with the corresponding entity.

    // 2. flag DW_AT_external attribute
    //      A DW_AT_external attribute, which is a flag, if the name of a variable is
    //      visible outside of its enclosing compilation unit.

    // 3. A subroutine entry may contain a DW_AT_main_subprogram attribute which is
    //      a flag whose presence indicates that the subroutine has been identified as the
    //      starting function of the program. If more than one subprogram contains this flag,
    //      any one of them may be the starting subroutine of the program.


    Dwarf_Die result;
    if(dwarf_child(fn->die, &result) != 0)
        return false;

    // went through parameters and local variables of the function
    do {
        switch (dwarf_tag(&result)) {
            case DW_TAG_formal_parameter:
            case DW_TAG_variable: {
                pst_parameter* param = add_param(fn);
                if(!pst_parameter_handle_dwarf(param, &result, fn)) {
                    del_param(param);
                }

                break;
            }
            case DW_TAG_GNU_call_site:
                pst_call_site_storage_handle_dwarf(&fn->call_sites, &result, fn);
                break;

                //              case DW_TAG_inlined_subroutine:
                //                  /* Recurse further down */
                //                  HandleFunction(&result);
                //                  break;
            case DW_TAG_lexical_block: {
                handle_lexical_block(fn, &result);
                break;
            }
            // Also handle:
            // DW_TAG_unspecified_parameters (unknown number of arguments i.e. fun(arg1, ...);
            // DW_AT_inline
            default:
                pst_log(SEVERITY_DEBUG, "Unknown TAG of function: 0x%X", dwarf_tag(&result));
                break;
        }
    } while(dwarf_siblingof(&result, &result) == 0);

    return true;
}

bool pst_function_unwind(pst_function* fn, Dwarf_Addr addr)
{
    fn->pc = addr;

    Dwfl_Line *dwline = dwfl_getsrc(fn->ctx->dwfl, addr);
    if(dwline != NULL) {
        Dwarf_Addr addr;
        const char* filename = dwfl_lineinfo (dwline, &addr, &fn->line, NULL, NULL, NULL);
        if(filename) {
            const char* str = strrchr(filename, '/');
            if(str && *str != 0) {
                str++;
            } else {
                str = filename;
            }
            fn->file = pst_strdup(str);
            fn->ctx->print(fn->ctx, "%s:%d", str, fn->line);
        } else {
            fn->ctx->print(fn->ctx, "%p", (void*)addr);
        }
    } else {
        fn->ctx->print(fn->ctx, "%p", (void*)addr);
    }

    const char* addrname = dwfl_module_addrname(fn->ctx->module, addr);
    char* demangle_name = NULL;
    if(addrname) {
        int status;
        demangle_name = abi::__cxa_demangle(addrname, NULL, NULL, &status);
        char* function_name = NULL;
        if(asprintf(&function_name, "%s%s", demangle_name ? demangle_name : addrname, demangle_name ? "" : "()") == -1) {
            pst_log(SEVERITY_ERROR, "Failed to allocate memory");
            return false;
        }
        fn->ctx->print(fn->ctx, " --> %s", function_name);

        char* str = strchr(function_name, '(');
        if(str) {
            *str = 0;
        }
        fn->name = pst_strdup(function_name);
        free(function_name);
    }

    if(demangle_name) {
        free(demangle_name);
    }

    return true;
}

void pst_function_init(pst_function* fn, pst_context* _ctx, __pst_function* _parent)
{
    list_node_init(&fn->node);

    // fields
    fn->lowpc = 0;
    fn->highpc = 0;
    fn->pc = 0;
    fn->die = NULL;
    fn->name = NULL;
    list_head_init(&fn->params);
    pst_call_site_storage_init(&fn->call_sites, _ctx);

    fn->sp = 0;
    fn->cfa = 0;
    memcpy(&fn->cursor, _ctx->curr_frame, sizeof(fn->cursor));
    fn->line = -1;
    fn->file = NULL;
    fn->parent = _parent;
    fn->frame = NULL;
    fn->ctx = _ctx;
    fn->allocated = false;
}

pst_function* pst_function_new(pst_context* _ctx, __pst_function* _parent)
{
    pst_function* fn = pst_alloc(pst_function);
    if(fn) {
        pst_function_init(fn, _ctx, _parent);
        fn->allocated = true;
    }

    return fn;
}

void pst_function_fini(pst_function* fn)
{
    clear(fn);

    if(fn->frame) {
        // use free here because it was allocate out of our control by libdw
        free(fn->frame);
    }

    if(fn->name) {
        pst_free(fn->name);
    }

    if(fn->file) {
        pst_free(fn->file);
    }

    if(fn->allocated) {
        pst_free(fn);
    }
}
