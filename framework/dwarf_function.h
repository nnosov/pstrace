/*
 * dwarf_function.h
 *
 *  Created on: Feb 1, 2020
 *      Author: nnosov
 */

#ifndef FRAMEWORK_DWARF_FUNCTION_H_
#define FRAMEWORK_DWARF_FUNCTION_H_

#include "context.h"
#include "dwarf_expression.h"
#include "list_head.h"
#include "dwarf_call_site.h"
#include "dwarf_parameter.h"

// -----------------------------------------------------------------------------------
// pst_function
// -----------------------------------------------------------------------------------
typedef struct __pst_function : public SC_ListNode {
    __pst_function(pst_context* _ctx, __pst_function* _parent) : ctx(_ctx) {
        pc = 0;
        line = -1;
        die = NULL;
        lowpc = 0;
        highpc = 0;
        memcpy(&cursor, _ctx->curr_frame, sizeof(cursor));
        parent = _parent;
        sp = 0;
        cfa = 0;
        frame = NULL;
    }

    ~__pst_function() {
        clear();
        if(frame) {
            free(frame);
        }
    }

    void clear();
    pst_parameter* add_param();
    void del_param(pst_parameter* p);
    pst_parameter* next_param(pst_parameter* p);

    pst_call_site* add_call_site(uint64_t target, const char* origin);
    void del_call_site(pst_call_site* st);
    pst_call_site* next_call_site(pst_call_site* st);
    pst_call_site* call_site_by_origin(const char* origin);
    pst_call_site* call_site_by_target(uint64_t target);

    bool unwind(Dwarf_Addr addr);
    bool handle_dwarf(Dwarf_Die* d);
    bool print_dwarf();
    bool handle_lexical_block(Dwarf_Die* result);
    bool handle_call_site(Dwarf_Die* result);
    pst_call_site* find_call_site(__pst_function* callee);

    bool get_frame();

    Dwarf_Addr                  lowpc;  // offset to start of the function against base address
    Dwarf_Addr                  highpc; // offset to the next address after the end of the function against base address
    Dwarf_Die*                  die;    // DWARF DIE containing definition of the function
    std::string                 name;   // function's name
    SC_ListHead                 params; // function's parameters

    // call-site
    SC_ListHead                 call_sites;     // Call-Site definitions
    SC_Dict                     mCallStToTarget; // map pointer to caller to call-site
    SC_Dict                     mCallStToOrigin; // map caller name to call-site

    unw_word_t                  pc;     // address between lowpc & highpc (plus base address offset). actually, currently executed command
    unw_word_t                  sp;     // SP register in function's frame
    unw_word_t                  cfa;    // CFA (Canonical Frame Adress) of the function
    unw_cursor_t                cursor; // copy of stack state of the function
    int                         line;   // line in code where function is defined
    std::string                 file;   // file name (DWARF Compilation Unit) where function is defined
    __pst_function*             parent; // parent function in call trace (caller)
    Dwarf_Frame*                frame;  // function's stack frame
    pst_context*                ctx;    // context of unwinding
} pst_function;


#endif /* FRAMEWORK_DWARF_FUNCTION_H_ */
