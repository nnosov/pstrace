/*
 * dwarf_call_site.cpp
 *
 *  Created on: Feb 1, 2020
 *      Author: nnosov
 */


#include <dwarf.h>
#include <elfutils/libdw.h>

#include "dwarf_call_site.h"
#include "dwarf_function.h"
#include "dwarf_utils.h"
#include "dwarf_utils.h"
#include "utils/hash_multimap.h"

// -----------------------------------------------------------------------------------
// pst_call_site_param
// -----------------------------------------------------------------------------------
void pst_call_site_param_init(pst_call_site_param* param)
{
    // fields
    param->param = NULL;
    param->name = NULL;
    param->value = 0;
    pst_dwarf_expr_init(&param->location);
    param->allocated = false;
}

pst_call_site_param* pst_call_site_param_new()
{
    pst_call_site_param* param = pst_alloc(pst_call_site_param);
    if(param) {
        pst_call_site_param_init(param);
        param->allocated = true;
    }

    return param;
}

void pst_call_site_param_fini(pst_call_site_param* param)
{
    if(param->name) {
        pst_free(param->name);
        param->name = NULL;
    }

    if(param->allocated) {
        pst_free(param);
    }
}

// -----------------------------------------------------------------------------------
// pst_call_site
// -----------------------------------------------------------------------------------
static pst_call_site_param* add_param(pst_call_site* site)
{
    pst_call_site_param* p = pst_call_site_param_new();
    pst_call_site_param_init(p);
    list_add_bottom(&site->params, &p->node);

    return p;
}

static void del_param(pst_call_site_param* p)
{
    list_del(&p->node);
    pst_call_site_param_fini(p);
}

static pst_call_site_param* next_param(pst_call_site* site, pst_call_site_param* p)
{
    list_node* n = (p == NULL) ? list_first(&site->params) : list_next(&p->node);
    pst_call_site_param* ret = NULL;
    if(n) {
        ret = list_entry(n, pst_call_site_param, node);
    }

    return ret;
}

pst_call_site_param* pst_call_site_find(pst_call_site* site, pst_dwarf_expr* expr)
{
    for(pst_call_site_param* param = next_param(site, NULL); param; param = next_param(site, param)) {
        if(pst_dwarf_expr_equal(&param->location, expr)) {
            return param;
        }
    }

    return NULL;
}

bool call_site_handle_dwarf(pst_call_site* site, Dwarf_Die* child)
{
    Dwarf_Attribute attr_mem;
    Dwarf_Attribute* attr;

    do {
        switch(dwarf_tag(child)) {
            case DW_TAG_GNU_call_site_parameter: {
                Dwarf_Addr pc;
                unw_get_reg(site->ctx->curr_frame, UNW_REG_IP,  &pc);

                // expression represent where callee parameter will be stored
                pst_call_site_param* param = add_param(site);
                if(dwarf_hasattr(child, DW_AT_location)) {
                    // determine location of parameter in stack/heap or CPU registers
                    attr = dwarf_attr(child, DW_AT_location, &attr_mem);
                    if(!handle_location(site->ctx, attr, &param->location, pc, NULL)) {
                        pst_log(SEVERITY_ERROR, "Failed to calculate DW_AT_location expression: %s", site->ctx->buff);
                        del_param(param);
                        return false;
                    }
                    pst_log(SEVERITY_DEBUG, "  DW_AT_location: %s", site->ctx->buff);
                }

                // expression represents call parameter's value
                if(dwarf_hasattr(child, DW_AT_GNU_call_site_value)) {
                    // handle value expression here
                    attr = dwarf_attr(child, DW_AT_GNU_call_site_value, &attr_mem);
                    pst_decl0(pst_dwarf_expr, loc);
                    if(handle_location(site->ctx, attr, &loc, pc, NULL)) {
                        param->value = loc.value;
                        pst_dwarf_expr_fini(&loc);
                        pst_log(SEVERITY_DEBUG, "  DW_AT_GNU_call_site_value:\"%s\" ==> 0x%lX", site->ctx->buff, param->value);
                    } else {
                        pst_log(SEVERITY_ERROR, "Failed to calculate DW_AT_location expression: %s", site->ctx->buff);
                        del_param(param);
                        pst_dwarf_expr_fini(&loc);
                        return false;
                    }
                }
                break;
            }
            default:
                break;
        }
    } while (dwarf_siblingof (child, child) == 0);

    return true;
}

void pst_call_site_init(pst_call_site* site, pst_context* c, uint64_t tgt, const char* orn)
{
    list_node_init(&site->node);

    site->target = tgt;
    if(orn) {
        site->origin = pst_strdup(orn);
    } else {
        site->origin = NULL;
    }
    site->die = NULL;
    list_head_init(&site->params);
    site->ctx = c;
    site->allocated = false;
}

pst_call_site* pst_call_site_new(pst_context* c, uint64_t tgt, const char* orn)
{
    pst_call_site* nc = pst_alloc(pst_call_site);
    if(nc) {
        pst_call_site_init(nc, c, tgt, orn);
        nc->allocated = true;
    }

    return nc;
}

void pst_call_site_fini(pst_call_site* site)
{
    if(site->origin) {
        pst_free(site->origin);
        site->origin = NULL;
    }

    if(site->allocated) {
        pst_free(site);
    }
}


// -----------------------------------------------------------------------------------
// pst_call_site_storage
// -----------------------------------------------------------------------------------

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
bool pst_call_site_storage_handle_dwarf(pst_call_site_storage* storage, Dwarf_Die* result, pst_function* fn)
{
    Dwarf_Die origin;
    Dwarf_Attribute attr_mem;
    Dwarf_Attribute* attr;

    pst_log(SEVERITY_DEBUG, "***** DW_TAG_GNU_call_site contents:");
    // reference to DIE which represents callee's parameter if compiler knows where it is at compile time
    const char* oname = NULL;
    if(dwarf_hasattr (result, DW_AT_abstract_origin) && dwarf_formref_die (dwarf_attr (result, DW_AT_abstract_origin, &attr_mem), &origin) != NULL) {
        oname = dwarf_diename(&origin);
        pst_log(SEVERITY_DEBUG, "DW_AT_abstract_origin: '%s'", oname);
    }

    // The call site may have a DW_AT_call_site_target attribute which is a DWARF expression.  For indirect calls or jumps where it is unknown at
    // compile time which subprogram will be called the expression computes the address of the subprogram that will be called.
    uint64_t target = 0;
    if(dwarf_hasattr (result, DW_AT_GNU_call_site_target)) {
        attr = dwarf_attr(result, DW_AT_GNU_call_site_target, &attr_mem);
        if(attr) {
            pst_decl0(pst_dwarf_expr, expr);
            if(handle_location(storage->ctx, &attr_mem, &expr, fn->info.pc, fn)) {
                target = expr.value;
                pst_log(SEVERITY_DEBUG, "DW_AT_GNU_call_site_target: %#lX", target);
            }
            pst_dwarf_expr_fini(&expr);
        }
    }

    if(target == 0 && oname == NULL) {
        pst_log(SEVERITY_ERROR, "Cannot determine both call-site target and origin");
        return false;
    }

    Dwarf_Die child;
    if(dwarf_child (result, &child) == 0) {
        pst_call_site* st = pst_call_site_storage_add(storage, target, oname);

        st->tail_call = (dwarf_hasattr(result, DW_AT_call_tail_call) != 0);

        // if DW_AT_low_pc attribute is specified, then it's value is actually PC in caller's frame (address of invocation of callee)
        if(dwarf_hasattr (result, DW_AT_low_pc) && dwarf_attr(result, DW_AT_low_pc, &attr_mem)) {
            if(!dwarf_formaddr(&attr_mem, &st->call_pc)) {
                pst_log(SEVERITY_DEBUG, "DW_AT_low_pc: %#lX", st->call_pc);
            }
        }

        if(!call_site_handle_dwarf(st, &child)) {
            pst_call_site_storage_del(storage, st);
            return false;
        }
    }

    return true;
}

pst_call_site* storage_call_site_by_origin(pst_call_site_storage* storage, const char* origin)
{
    pst_call_site* ret = NULL;
    hash_node* node = hash_find(&storage->cs_to_origin, origin, strlen(origin));
    if(node) {
        ret = hash_entry(node, pst_call_site, org_node);
    }

    return ret;
}

pst_call_site* storage_call_site_by_target(pst_call_site_storage* storage, uint64_t target)
{
    pst_call_site* ret = NULL;
    hash_node* node = hash_find(&storage->cs_to_target, (char*)&target, sizeof(target));
    if(node) {
        ret = hash_entry(node, pst_call_site, tgt_node);
    }

    return ret;
}

pst_call_site* pst_call_site_storage_add(pst_call_site_storage* storage, uint64_t target, const char* origin)
{
    pst_new(pst_call_site, st, storage->ctx, target, origin);
    list_add_bottom(&storage->call_sites, &st->node);

    if(target) {
        hash_add(&storage->cs_to_target, &st->tgt_node, &target, sizeof(target));
    } else if(origin) {
        hash_add(&storage->cs_to_origin, &st->org_node, origin, strlen(origin));
    }

    return st;
}

void pst_call_site_storage_del(pst_call_site_storage* storage, pst_call_site* st)
{
    hash_node* node = NULL;
    list_del(&st->node);

    if(st->target) {
       node = hash_find(&storage->cs_to_target, &st->target, sizeof(st->target));
    } else if(st->origin) {
        node = hash_find(&storage->cs_to_origin, st->origin, strlen(st->origin));
    }

    if(node) {
        hash_del(node);
    }

    pst_free(st);
}

pst_call_site* pst_call_site_storage_find(pst_call_site_storage* storage, pst_function* callee)
{
    uint64_t start_pc = storage->ctx->base_addr + callee->info.lowpc;
    pst_call_site* cs = storage_call_site_by_target(storage, start_pc);
    if(!cs) {
        cs = storage_call_site_by_origin(storage, callee->info.name);
    }

    return cs;
}

void pst_call_site_storage_init(pst_call_site_storage* storage, pst_context* ctx)
{
    storage->ctx = ctx;
    list_head_init(&storage->call_sites);
    hash_head_init(&storage->cs_to_target, HASH_MIN_SHIFT, 0, 0);
    hash_head_init(&storage->cs_to_origin, HASH_MIN_SHIFT, 0 ,0);
    storage->allocated = false;
}

pst_call_site_storage* pst_call_site_storage_new(pst_context* ctx)
{
    pst_call_site_storage* ns = pst_alloc(pst_call_site_storage);
    if(ns) {
        pst_call_site_storage_init(ns, ctx);
        ns->allocated = true;
    }

    return ns;
}

void pst_call_site_storage_fini(pst_call_site_storage* storage)
{
    pst_call_site*  site = NULL;
    struct list_node  *pos, *tn;
    list_for_each_entry_safe(site, pos, tn, &storage->call_sites, node) {
        list_del(&site->node);
        pst_call_site_fini(site);
    }

    if(storage->allocated) {
        pst_free(storage);
    }
}
