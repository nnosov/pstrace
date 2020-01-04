/*
 * sysutils.h
 *
 *  Created on: Dec 28, 2019
 *      Author: nnosov
 */

#ifndef SC_SYSUTILS_H_
#define SC_SYSUTILS_H_

//system
#include <string>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ucontext.h>

using std::string;

// stack trace string buffer
extern char __stack_trace[8192];

// libdw-based trace call stack implementation
const char* LibdwTraceCallStack(ucontext_t* uctx);

char* GetExecutableName(char* path, uint32_t size);

#endif /* SC_SYSUTILS_H_ */
