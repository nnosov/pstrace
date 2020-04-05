/*
 * dwarf_call_site.h
 *
 *  Created on: Feb 1, 2020
 *      Author: nnosov
 */

#ifndef __PST_DWARF_CALL_SITE_H__
#define __PST_DWARF_CALL_SITE_H__

#include <inttypes.h>
#include <elfutils/libdw.h>

#include "../src/dwarf/dwarf_expression.h"
#include "../utils/hash_multimap.h"
#include "../utils/list_head.h"
#include "context.h"

typedef struct __pst_callee_info {
    Dwarf_Addr      target;
    char*           origin;
} pst_callee_info;


// -----------------------------------------------------------------------------------
// DW_TAG_call_site_parameter
// -----------------------------------------------------------------------------------
typedef struct __pst_call_site_param {
    list_node           node;   // uplink. !!! must be first !!!

    Dwarf_Die*          param;      // reference to parameter DIE in callee (DW_AT_call_parameter)
    char*               name;       // name of parameter if present (DW_AT_name)
    pst_dwarf_expr      location;   // DWARF stack containing location expression
    uint64_t            value;      // parameter's value
    bool                allocated;  // whether this object was allocated or not
} pst_call_site_param;

void pst_call_site_param_init(pst_call_site_param* param);
pst_call_site_param* pst_call_site_param_new();
void pst_call_site_param_fini(pst_call_site_param* param);


// -----------------------------------------------------------------------------------
// DW_TAG_call_site
// -----------------------------------------------------------------------------------
typedef struct __pst_call_site {
    list_node       node;       // uplink to list of call-sites
    hash_node       tgt_node;   // uplink to call-site by node dictionary
    hash_node       org_node;   // uplink to call-site by origin dictionary

    uint64_t        target;     // pointer to callee function (it's Low PC + base address)
    char*           origin;     // name of callee function
    Dwarf_Addr      call_pc;    // address of 'call' instruction to callee inside of caller
    bool            tail_call;  // whether this a tail-call (jump) or normal call 'call'
    Dwarf_Die*      die;        // DIE of function for which this call site has parameters
    list_head       params;     // list of parameters and their values
    pst_context*    ctx;        // execution context
    bool            allocated;  // whether this object was allocated or not
} pst_call_site;
void pst_call_site_init(pst_call_site* site, pst_context* context, uint64_t target, const char* origin);
pst_call_site* pst_call_site_new(pst_context* context, uint64_t target, const char* origin);
void pst_call_site_fini(pst_call_site* site);

pst_call_site_param* pst_call_site_find(pst_call_site* site, pst_dwarf_expr* expr);
bool pst_call_site_handle_dwarf(pst_call_site* site, Dwarf_Die* child);

typedef struct pst_function pst_function;
// -----------------------------------------------------------------------------------
// storage for all of  function's call sites
// -----------------------------------------------------------------------------------
typedef struct __pst_call_site_storage {
    pst_context*        ctx;
    list_head           call_sites;     // Call-Site definitions
    hash_head           cs_to_target;   // map pointer to caller to call-site
    hash_head           cs_to_origin;   // map caller name to call-site
    bool                allocated;      // whether this object was allocated or not
} pst_call_site_storage;

void pst_call_site_storage_init(pst_call_site_storage* storage, pst_context* ctx);
pst_call_site_storage* pst_call_site_storage_new(pst_context* ctx);
void pst_call_site_storage_fini(pst_call_site_storage* storage);

bool pst_call_site_storage_handle_dwarf(pst_call_site_storage* storage, Dwarf_Die* result, pst_function* info);
pst_call_site* pst_call_site_storage_find(pst_call_site_storage* storage, pst_function* callee);
pst_call_site* pst_call_site_storage_add(pst_call_site_storage* storage, uint64_t target, const char* origin);
void pst_call_site_storage_del(pst_call_site_storage* storage, pst_call_site* st);
void pst_call_site_storage_del(pst_call_site_storage* storage, pst_call_site* st);


#endif /* __PST_DWARF_CALL_SITE_H__ */
