/*
 * dwarf_function.cpp
 *
 *  Created on: Feb 1, 2020
 *      Author: nnosov
 */

#include "dwarf_function.h"


// -----------------------------------------------------------------------------------
// pst_function
// -----------------------------------------------------------------------------------
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

    for(pst_call_site* p = (pst_call_site*)call_sites.First(); p; p = (pst_call_site*)call_sites.First()) {
        call_sites.Remove(p);
        delete p;
    }

    mCallStToTarget.Clean();
    mCallStToTarget.Clean();
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

pst_call_site* __pst_function::add_call_site(uint64_t target, const char* origin)
{
    pst_call_site* st = new pst_call_site(ctx, target, origin);
    call_sites.InsertLast(st);
    if(target) {
        mCallStToTarget.Insert(&target, sizeof(target), st);
    }

    if(origin) {
        mCallStToOrigin.Insert(origin, strlen(origin), st);
    }

    return st;
}
void __pst_function::del_call_site(pst_call_site* st)
{
    call_sites.Remove(st);
    if(st->target && mCallStToTarget.Lookup(&st->target, sizeof(st->target))) {
        mCallStToTarget.Remove();
    }

    if(st->origin && mCallStToTarget.Lookup(st->origin, strlen(st->origin))) {
        mCallStToTarget.Remove();
    }

    delete st;
}
pst_call_site* __pst_function::next_call_site(pst_call_site* st)
{
    pst_call_site* next = NULL;
    if(st) {
        next = (pst_call_site*)call_sites.First();
    } else {
        next = (pst_call_site*)call_sites.Next(st);
    }

    return next;
}

pst_call_site* __pst_function::call_site_by_origin(const char* origin)
{
    return (pst_call_site*)mCallStToOrigin.Lookup(origin, strlen(origin));
}

pst_call_site* __pst_function::call_site_by_target(uint64_t target)
{
    return (pst_call_site*)mCallStToTarget.Lookup(&target, sizeof(target));
}

pst_call_site* __pst_function::find_call_site(pst_function* callee)
{
    uint64_t start_pc = ctx->base_addr + callee->lowpc;
    pst_call_site* cs = (pst_call_site*)mCallStToTarget.Lookup(&start_pc, sizeof(start_pc));
    if(!cs) {
        cs = (pst_call_site*)mCallStToOrigin.Lookup(callee->name.c_str(), strlen(callee->name.c_str()));
    }

    return cs;
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
    //ctx->print(ctx, "%s:%d: ", file.c_str(), line);

    // handle return parameter and be safe if function haven't parameters (for example, dwar info for function is absent)
    pst_parameter* param = next_param(NULL);
    if(param && param->is_return) {
        // print return value type, function name and start list of parameters
        param->print_dwarf();
        ctx->print(ctx, " %s(", name.c_str());
        param = next_param(param);
    } else {
        ctx->print(ctx, "%s(", name.c_str());
    }

    bool first = true; bool start_variable = false;
    for(; param; param = next_param(param)) {
        if(param->is_return) {
            // print return value type, function name and start list of parameters
            param->print_dwarf();
            ctx->print(ctx, " %s(", name.c_str());
            continue;
        }

        if(param->is_variable) {
            if(!start_variable) {
                ctx->print(ctx, ")%s\n", at);
                ctx->print(ctx, "{\n");
                start_variable = true;
            }
            if(param->line) {
                ctx->print(ctx, "%04u:   ", param->line);
            } else {
                ctx->print(ctx, "        ");
            }
            param->print_dwarf();
            ctx->print(ctx, ";\n");
        } else {
            if(first) {
                first = false;
            } else {
                ctx->print(ctx, ", ");
            }
            param->print_dwarf();
        }
    }

    if(!start_variable) {
        ctx->print(ctx, ");%s\n", at);
    } else {
        ctx->print(ctx, "}\n");
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
                    if(!param->handle_dwarf(&child, this)) {
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

    ctx->log(SEVERITY_DEBUG, "***** DW_TAG_GNU_call_site contents:");
    // reference to DIE which represents callee's parameter if compiler knows where it is at compile time
    const char* oname = NULL;
    if(dwarf_hasattr (result, DW_AT_abstract_origin) && dwarf_formref_die (dwarf_attr (result, DW_AT_abstract_origin, &attr_mem), &origin) != NULL) {
        oname = dwarf_diename(&origin);
        ctx->log(SEVERITY_DEBUG, "\tDW_AT_abstract_origin: '%s'", oname);
    }

    // The call site may have a DW_AT_call_site_target attribute which is a DWARF expression.  For indirect calls or jumps where it is unknown at
    // compile time which subprogram will be called the expression computes the address of the subprogram that will be called.
    uint64_t target = 0;
    if(dwarf_hasattr (result, DW_AT_GNU_call_site_target)) {
        attr = dwarf_attr(result, DW_AT_GNU_call_site_target, &attr_mem);
        if(attr) {
            pst_dwarf_expr expr(ctx->alloc);
            if(handle_location(ctx, &attr_mem, expr, pc, this)) {
                target = expr.value;
                ctx->log(SEVERITY_DEBUG, "\tDW_AT_GNU_call_site_target: %#lX", target);
            }
        }
    }

    if(target == 0 && oname == NULL) {
        ctx->log(SEVERITY_ERROR, "Cannot determine both call-site target and origin");
        return false;
    }

    Dwarf_Die child;
    if(dwarf_child (result, &child) == 0) {
        pst_call_site* st = add_call_site(target, oname);
        if(!st->handle_dwarf(&child)) {
            del_call_site(st);
            return false;
        }
    }

    return true;
}

bool __pst_function::handle_dwarf(Dwarf_Die* d)
{
    die = d;
    get_frame();

    Dwarf_Attribute attr_mem;
    Dwarf_Attribute* attr;

    // get list of offsets from process base address of continuous memory ranges where function's code resides
//  if(dwarf_haspc(d, pc)) {
        dwarf_lowpc(d, &lowpc);
        dwarf_highpc(d, &highpc);
//  } else {
//      ctx->log(SEVERITY_ERROR, "Function's '%s' DIE hasn't definitions of memory offsets of function's code", dwarf_diename(d));
//      return false;
//  }

    unw_proc_info_t info;
    unw_get_proc_info(&cursor, &info);
    ctx->clean_print(ctx);

    ctx->log(SEVERITY_INFO, "Function %s(...): LOW_PC = %#lX, HIGH_PC = %#lX, offset from base address: 0x%lX, START_PC = 0x%lX, offset from start of function: 0x%lX",
            dwarf_diename(d), lowpc, highpc, pc - ctx->base_addr, info.start_ip, info.start_ip - ctx->base_addr);
    ctx->print_registers(ctx, 0x0, 0x10);
    ctx->log(SEVERITY_INFO, "Function %s(...): CFA: %#lX %s", dwarf_diename(d), parent ? parent->sp : 0, ctx->buff);
    ctx->log(SEVERITY_INFO, "Function %s(...): %s", dwarf_diename(d), ctx->buff);

    // determine function's stack frame base
    attr = dwarf_attr(die, DW_AT_frame_base, &attr_mem);
    if(attr) {
        if(dwarf_hasform(attr, DW_FORM_exprloc)) {
            Dwarf_Op *expr;
            size_t exprlen;
            if (dwarf_getlocation (attr, &expr, &exprlen) == 0) {
                ctx->print_expr(ctx, expr, exprlen, attr);
                pst_decl(pst_dwarf_stack, stack, ctx);
                if(stack.calc(&stack, expr, exprlen, attr, this)) {
                    uint64_t value;
                    if(stack.get_value(&stack, &value)) {
                        ctx->log(SEVERITY_DEBUG, "DW_AT_framebase expression: \"%s\" ==> 0x%lX", ctx->buff, value);
                    } else {
                        ctx->log(SEVERITY_ERROR, "Failed to get value of calculated DW_AT_framebase expression: %s", ctx->buff);
                    }
                } else {
                    ctx->log(SEVERITY_ERROR, "Failed to calculate DW_AT_framebase expression: %s", ctx->buff);
                }
                pst_dwarf_stack_fini(&stack);
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
            ctx->log(SEVERITY_ERROR, "Failed to handle return parameter type for function %s(...)", name.c_str());
            del_param(ret_p);
        }
    } else {
        ret_p->add_type("void", 0);
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
    if(dwarf_child(die, &result) != 0)
        return false;

    // went through parameters and local variables of the function
    do {
        switch (dwarf_tag(&result)) {
            case DW_TAG_formal_parameter:
            case DW_TAG_variable: {
                pst_parameter* param = add_param();
                if(!param->handle_dwarf(&result, this)) {
                    del_param(param);
                }

                break;
            }
            case DW_TAG_GNU_call_site:
                handle_call_site(&result);
                break;

                //              case DW_TAG_inlined_subroutine:
                //                  /* Recurse further down */
                //                  HandleFunction(&result);
                //                  break;
            case DW_TAG_lexical_block: {
                handle_lexical_block(&result);
                break;
            }
            // Also handle:
            // DW_TAG_unspecified_parameters (unknown number of arguments i.e. fun(arg1, ...);
            // DW_AT_inline
            default:
                ctx->log(SEVERITY_DEBUG, "Unknown TAG of function: 0x%X", dwarf_tag(&result));
                break;
        }
    } while(dwarf_siblingof(&result, &result) == 0);

    return true;
}

bool __pst_function::unwind(Dwarf_Addr addr)
{
    pc = addr;

    Dwfl_Line *dwline = dwfl_getsrc(ctx->dwfl, addr);
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
            ctx->print(ctx, "%s:%d", str, line);
        } else {
            ctx->print(ctx, "%p", (void*)addr);
        }
    } else {
        ctx->print(ctx, "%p", (void*)addr);
    }

    const char* addrname = dwfl_module_addrname(ctx->module, addr);
    char* demangle_name = NULL;
    if(addrname) {
        int status;
        demangle_name = abi::__cxa_demangle(addrname, NULL, NULL, &status);
        char* function_name = NULL;
        if(asprintf(&function_name, "%s%s", demangle_name ? demangle_name : addrname, demangle_name ? "" : "()") == -1) {
            ctx->log(SEVERITY_ERROR, "Failed to allocate memory");
            return false;
        }
        ctx->print(ctx, " --> %s", function_name);

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

bool __pst_function::get_frame()
{
    // get CFI (Call Frame Information) for current module
    // from handle_cfi()
    Dwarf_Addr mod_bias = 0;
    Dwarf_CFI* cfi = dwfl_module_eh_cfi(ctx->module, &mod_bias); // rty .eh_cfi first
    if(!cfi) { // then try .debug_fame second
        cfi = dwfl_module_dwarf_cfi(ctx->module, &mod_bias);
    }
    if(!cfi) {
        ctx->log(SEVERITY_ERROR, "Cannot find CFI for module");
        return false;
    }

    // get frame of CFI for address
    int result = dwarf_cfi_addrframe (cfi, pc - mod_bias, &frame);
    if (result != 0) {
        ctx->log(SEVERITY_ERROR, "Failed to find CFI frame for module");
        return false;
    }

    // setup context to match frame
    ctx->frame = frame;

    // get return register and PC range for function
    Dwarf_Addr start = pc;
    Dwarf_Addr end = pc;
    bool signalp;
    int ra_regno = dwarf_frame_info (frame, &start, &end, &signalp);
    if(ra_regno >= 0) {
        start += mod_bias;
        end += mod_bias;
        reginfo info; info.regno = ra_regno;
        dwfl_module_register_names(ctx->module, regname_callback, &info);
        ctx->log(SEVERITY_INFO, "Function %s(...): '.eh/debug frame' info: PC range:  => [%#" PRIx64 ", %#" PRIx64 "], return register: %s, in_signal = %s",
                name.c_str(), start, end, info.regname, signalp ? "true" : "false");
    } else {
        ctx->log(SEVERITY_WARNING, "Return address register info unavailable (%s)", dwarf_errmsg(0));
    }

    // finally get CFA (Canonical Frame Address)
    // Point cfa_ops to dummy to match print_detail expectations.
    // (nops == 0 && cfa_ops != NULL => "undefined")
    Dwarf_Op dummy;
    Dwarf_Op *cfa_ops = &dummy;
    size_t cfa_nops;
    if(dwarf_frame_cfa(frame, &cfa_ops, &cfa_nops)) {
        ctx->log(SEVERITY_ERROR, "Failed to get CFA for frame");
        return false;
    }

    ctx->print_expr(ctx, cfa_ops, cfa_nops, NULL);
    pst_decl(pst_dwarf_stack, stack, ctx);
    if(stack.calc(&stack, cfa_ops, cfa_nops, NULL, this) && stack.get_value(&stack, &cfa)) {
        //ctx.sp = v;
        ctx->log(SEVERITY_INFO, "Function %s(...): CFA expression: %s ==> %#lX", name.c_str(), ctx->buff, cfa);

        // setup context to match CFA for frame
        ctx->cfa = cfa;
    } else {
        ctx->log(SEVERITY_ERROR, "Failed to calculate CFA expression");
    }

    pst_dwarf_stack_fini(&stack);

    return true;
}

