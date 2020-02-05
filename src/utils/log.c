
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>

#include "log.h"

const char * const severity_map[] = {
		" [DEBUG]  : ",
        " [INFO]   : ",
        " [WARNING]: ",
        " [ERROR]  : "
};

static void format_string(pst_log* log, const char* fmt, va_list args)
{
    log->mStringLen += vsnprintf(log->mString + log->mStringLen, log->mStringSize - log->mStringLen, fmt, args);
}

static void format_prefix(pst_log* log, SC_LogSeverity severity)
{
    time_t rawTime;
    struct tm * timeinfo;
    time(&rawTime);
    timeinfo = gmtime(&rawTime);

    log->mStringLen += strftime(log->mString, sizeof(log->mString), "%d-%m-%Y %H:%M:%S", timeinfo);
    strncpy(log->mString + log->mStringLen, severity_map[(int)severity], strlen(severity_map[(int)severity]));
    log->mStringLen += strlen(severity_map[(int)severity]);
}

static void format_postfix(pst_log* log)
{
    if((log->mStringLen + 2) < log->mStringSize) {
        log->mString[log->mStringLen++] = '\n';
        log->mString[log->mStringLen++] = 0;
    }
}

//variable argument number logging
static void log_base(pst_log* log, SC_LogSeverity severity, const char* fmt, ...)
{
    if (severity < log->mCurrentSeverity)
        return;

    pthread_mutex_lock(&log->mLock);

    if(!log->is_opened(log)) {
        log->open(log);
    }

    log->mStringLen = 0;
    format_prefix(log, severity);
    va_list args;
    va_start(args, fmt);
    format_string(log, fmt, args);
    va_end(args);
    format_postfix(log);
    log->send_message(log, severity);

    pthread_mutex_unlock(&log->mLock);
}

static void log_init_base(pst_log* plog, const char* source)
{
    // methods
    plog->log = log_base;

    // fields
    plog->mpSource = 0;
    if(source) {
        plog->mpSource = strdup(source);
    }
    plog->mString[0] = 0;
    plog->mStringSize = sizeof(plog->mString);
    plog->mStringLen = 0;
    plog->mCurrentSeverity = SEVERITY_DEBUG;
    pthread_mutex_init(&plog->mLock, NULL);
    plog->child = 0;
}

void pst_log_fini(pst_log* log)
{
    pthread_mutex_destroy(&log->mLock);
    log->close(log);
    if(log->mpSource) {
        free(log->mpSource);
        log->mpSource = 0;
    }
}


//
// Console logger implementation
//
#define RED     "\e[0;31m"
#define GREEN   "\e[0;32m"
#define YELLOW  "\e[1;33m"
#define NC      "\e[0m" // No Color

//actually sends message to the source
static void send_msg_console(pst_log* log, SC_LogSeverity severity)
{
    const char* color = NC;
    switch(severity) {
        case SEVERITY_DEBUG:
            color = NC;
            break;
        case SEVERITY_ERROR:
            color = RED;
            break;
        case SEVERITY_INFO:
            color = GREEN;
            break;
        case SEVERITY_WARNING:
            color = YELLOW;
            break;
        default:
            color = NC;
            break;
    }

    fprintf(stderr, "%s%s%s", color, log->mString, NC);
}

static void close_console(pst_log* log) {
    // do nothing
}

static bool open_console(pst_log* log) {
    return true;
}

static bool is_opened_console(pst_log* log) {
    return true;
}

void pst_log_init_console(pst_log* log)
{
    log_init_base(log, NULL);
    log->close = close_console;
    log->open = open_console;
    log->is_opened = is_opened_console;
    log->send_message = send_msg_console;
}

//
// Log to file
//

typedef struct _file_spec {
    FILE*       fd;
    uint64_t    max_bytes;
    uint64_t    num_bytes;
    char*       dir;
    char*       fname;
} file_spec;

//open file for writing
static bool open_file(pst_log* log)
{
    file_spec* fsp = (file_spec*)log->child;
    if(!fsp) {
        fprintf(stderr, "Wrong initialization of File-based logger\n");
        return false;
    }

    if(!fsp->fd) {
        // extract log and path names
        uint32_t namelen = strlen(log->mpSource);
        if(namelen) {
            const char* ptr = strrchr(log->mpSource, '/');
            uint32_t pathlen = 0;
            if(ptr) {
                pathlen = ptr - log->mpSource + 1;
                namelen -= pathlen + 1;
            }

            fsp->dir = strndup(log->mpSource, pathlen);
            fsp->fname = strndup(log->mpSource + pathlen, namelen);

            //open the file finally
            fsp->fd = fopen(log->mpSource, "a+");
        }
    }

    return (fsp->fd != 0);
}

static void close_file(pst_log* log)
{
    file_spec* fsp = (file_spec*)log->child;
    if(!fsp) {
        fprintf(stderr, "Wrong initialization of File-based logger while close\n");
        return;
    }

    if(fsp->fd) {
        fclose(fsp->fd);
    }

    fsp->fd = 0;
    fsp->max_bytes = 0;
}

static bool is_file_opened(pst_log* log)
{
    file_spec* fsp = (file_spec*)log->child;
    if(!fsp) {
        fprintf(stderr, "Wrong initialization of File-based logger while check open\n");
        return 0;
    }
    return (fsp->fd != 0);
}

static void send_msg_file(pst_log* log, SC_LogSeverity severity)
{
    file_spec* fsp = (file_spec*)log->child;
    if(!fsp) {
        fprintf(stderr, "Wrong initialization of File-based logger while check send message\n");
        return;
    }
    if(fsp->fd && fsp->num_bytes < fsp->max_bytes) {
        if(fputs(log->mString, fsp->fd) >= 0) {
            fflush(fsp->fd);
            fsp->num_bytes += log->mStringLen;
        } else {
            fclose(fsp->fd);
            fsp->fd = 0;
        }
    }
}

void pst_log_init_file(pst_log* log, const char* path, uint64_t max_bytes)
{
    log_init_base(log, path);
    file_spec* fsp = (file_spec*)malloc(sizeof(file_spec));
    fsp->fd = 0;
    fsp->max_bytes = max_bytes;
    fsp->num_bytes = 0;
    log->child = fsp;

    log->close = close_file;
    log->open = open_file;
    log->is_opened = is_file_opened;
    log->send_message = send_msg_file;

}
