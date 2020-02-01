//system
#include <iostream>
#include <stdlib.h>
#include <fcntl.h>

#include "context.h"
#include "sysutils.h"

typedef enum {
	DEF_1 = 1,
	DEF_2,
	DEF_3
} my_int;

void Fun2(int arg1, uint32_t arg2)
{
	int* ptr = NULL;
	*ptr = arg1;
	void* ptr2 = ptr+1;
	printf("%p\n", ptr2);
}

uint32_t Fun1(const int arg1, my_int arg2, uint32_t arg3)
{
	int my_local = arg1 + 2;
	printf("%d\n", my_local);
	Fun2(my_local, arg3);
	return arg2;
}

#include <ucontext.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <signal.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef void (*sig_handler_t)(int sig);

void SetSignalHandler(sig_handler_t handler = 0);

void SigusrHandler(int sig)
{
    // just do nothing on SIGUSR1 & SIGUSR2
    SetSignalHandler(SigusrHandler);
}

volatile sig_atomic_t fatal_error_in_progress = 0;

void FatalSignalHandler(int sig, siginfo_t* info, void* context)
{
	// Since this handler is established for more than one kind of signal,
	// it might still get invoked recursively by delivery of some other kind
	// of signal.  Use a static variable to keep track of that.
    if (fatal_error_in_progress) {
	    raise (sig);
    }

    fatal_error_in_progress = 1;

    logger.log(&logger, SEVERITY_ERROR, "%s signal handled", strsignal(sig));
    bool ret = false;
    pst_handler handler((ucontext_t*)context);

	if((context != 0) && (sig == SIGSEGV || sig == SIGABRT || sig == SIGBUS || sig == SIGFPE))
	{
	    ret = handler.unwind();
    }

	if(ret) {
	    logger.log(&logger, SEVERITY_INFO, "%s", handler.ctx.buff);
	    handler.handle_dwarf();
	    handler.print_dwarf();
	    logger.log(&logger, SEVERITY_INFO, "%s", handler.ctx.buff);
	} else {
	    logger.log(&logger, SEVERITY_ERROR, "No stack trace obtained");
	}

    // comment out line below to prevent coredump
    // exit(EXIT_FAILURE);

	// Now reraise the signal.
	// We reactivate the signal’s default handling, which is to terminate the process.
	// We could just call exit or abort, but reraising the signal sets the return status
	// from the process correctly.
	signal (sig, SIG_DFL);
	raise (sig);
}

void SignalHandler(int sig)
{
//	if (handled_error_in_progress)
//		raise (sig);
//	handled_error_in_progress = 1;

//	const char* str_sig = strsignal(sig);
//	logger->Log(SEVERITY_INFO, "%s received.", str_sig);
	if(sig == SIGUSR1) {
		logger.log(&logger, SEVERITY_INFO, "Maintenance mode enabled.");
	} else if(sig == SIGUSR2) {
		logger.log(&logger, SEVERITY_INFO, "Maintenance mode disabled.");
	}
//	signal (sig, SIG_DFL);
//	raise(sig);
	// reset to our handler back
	SetSignalHandler();
}

#include <sys/time.h>
#include <sys/resource.h>

void SetSignalHandler(sig_handler_t handler)
{
	//signals set
	sigset_t ss;
	sigfillset(&ss);

	//remove previous handlers
//	sigdelset(&ss, SIGINT);
	sigdelset(&ss, SIGUSR1);
	sigdelset(&ss, SIGUSR2);
	sigdelset(&ss, SIGSEGV);
	sigdelset(&ss, SIGABRT);
	sigdelset(&ss, SIGBUS);
	sigdelset(&ss, SIGFPE);
	sigdelset(&ss, SIGCHLD);
	sigdelset(&ss, SIGTERM);		//without replacement

	//signal action
	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sa.sa_mask = ss;

	// cleanup
	sa.sa_handler = NULL;
	sa.sa_sigaction = NULL;

//	//set interrupt signal
//	sa.sa_handler = SignalHandler;
//	sigaction(SIGINT, &sa, 0);

	//set stop signal
	sa.sa_handler = (handler == NULL) ? SignalHandler : handler;
	sigaction(SIGUSR1, &sa, 0);

	sa.sa_handler = SignalHandler;
	sigaction(SIGUSR2, &sa, 0);

	// cleanup
	sa.sa_handler = NULL;
	sa.sa_sigaction = NULL;

	//set SIGSEGV
	sa.sa_sigaction = FatalSignalHandler;
	sigaction(SIGSEGV, &sa, 0);

	//set SIGABRT
	sa.sa_sigaction = FatalSignalHandler;
	sigaction(SIGABRT, &sa, 0);

	//set SIGBUS
	sa.sa_sigaction = FatalSignalHandler;
	sigaction(SIGBUS, &sa, 0);

	//set FPE signal
	sa.sa_sigaction = FatalSignalHandler;
	sigaction(SIGFPE, &sa, 0);

	//enable signals set
	sigprocmask(SIG_BLOCK, &ss, 0);

	//set core dump size to 1Gb
	struct rlimit limit;
	limit.rlim_cur = 1073741824;
	limit.rlim_max = 1073741824;
	if (setrlimit(RLIMIT_CORE, &limit))
	{
		logger.log(&logger, SEVERITY_DEBUG, "Failed to set limit for core dump file size");
	}
}

void ResetSignalHandler() {
	signal(SIGINT, SIG_DFL);
	signal(SIGABRT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
    signal(SIGUSR1, SIG_DFL);
    signal(SIGUSR2, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
}

int main(int argc, char* argv[])
{
    pst_log_init_console(&logger);
    //logger->SetCurrentSeverity(SEVERITY_INFO);
    SetSignalHandler(SigusrHandler);

    Fun1(1, DEF_2, 5);

    return 0;
}
