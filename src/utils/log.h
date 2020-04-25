#ifndef __PST_LOG_H_
#define __PST_LOG_H_

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <limits.h>
#include <stdbool.h>

// Log message severity
typedef enum {
    SEVERITY_DEBUG = 0,
    SEVERITY_INFO,
    SEVERITY_WARNING,
    SEVERITY_ERROR,
    //----------------
    SEVERITY_MAX
} SC_LogSeverity;

#define LOG_BUFF_SIZE       (1024*1024)

typedef struct __pst_log {
    // methods
    void (*close) (struct __pst_log* log);
    bool (*open) (struct __pst_log* log);
    bool (*is_opened) (struct __pst_log* log);
    void (*log) (struct __pst_log* log, SC_LogSeverity severity, const char* fmt, ...);
    void (*send_message) (struct __pst_log* log, SC_LogSeverity severity);

    // fields
    char*                       mpSource;               // source for store log messages (file, IP:port or something else)

    char                        mString[LOG_BUFF_SIZE]; // formatted string to be printed
    uint32_t                    mStringSize;            // size of the 'mString' buffer
    uint32_t                    mStringLen;             // length of the formatted string

    SC_LogSeverity            	mCurrentSeverity;       // maximum severity value to be logged
    pthread_mutex_t             mLock;

    void*                       child;
} pst_logger;

void pst_log_init_console(pst_logger* log);
void pst_log_init_file(pst_logger* log, const char* path, uint64_t max_bytes);
void pst_log_fini(pst_logger* log);

#endif /* __PST_LOG_H_ */
