#ifndef __PST_LOG_H_
#define __PST_LOG_H_

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <limits.h>

// Log message severity
enum SC_LogSeverity {
    SEVERITY_DEBUG = 0,
    SEVERITY_INFO,
    SEVERITY_WARNING,
    SEVERITY_ERROR,
    //----------------
    SEVERITY_MAX
};

#define LOG_BUFF_SIZE       (1024*1024)

typedef struct pst_log {
    // methods
    void (*close) (pst_log* log);
    bool (*open) (pst_log* log);
    bool (*is_opened) (pst_log* log);
    void (*log) (pst_log* log, SC_LogSeverity severity, const char* fmt, ...);
    void (*send_message) (pst_log* log, SC_LogSeverity severity);

    // fields
    char*                       mpSource;               // source for store log messages (file, IP:port or something else)

    char                        mString[LOG_BUFF_SIZE]; // formatted string to be printed
    uint32_t                    mStringSize;            // size of the 'mString' buffer
    uint32_t                    mStringLen;             // length of the formatted string

    SC_LogSeverity            	mCurrentSeverity;       // maximum severity value to be logged
    pthread_mutex_t             mLock;

    void*                       child;
} pst_log;

void pst_log_init_console(pst_log* log);
void pst_log_init_file(pst_log* log, const char* path, uint64_t max_bytes = 64 * 1024 * 1024/* 64Mb */);
void pst_log_fini(pst_log* log);

#endif /* __PST_LOG_H_ */
