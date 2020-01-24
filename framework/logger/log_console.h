#ifndef SC_LOGCONSOLE_H
#define SC_LOGCONSOLE_H

#include "log.h"

#define RED     "\e[0;31m"
#define GREEN   "\e[0;32m"
#define YELLOW  "\e[1;33m"
#define NC      "\e[0m" // No Color

class SC_LogConsole : public SC_LogBase {
public:

    SC_LogConsole(uint32_t id, const char* source) : SC_LogBase(id, source) { }

    //open file for writing
    inline virtual bool Open() {
        return true;
    }

    //close file opened
    inline virtual void Close() { }

    //whether file already opened or not
    inline virtual bool IsOpened() {
        return true;
    }
protected:
    inline virtual void SendMessage(const char* msg, SC_LogSeverity severity) {
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

        fprintf(stderr, "%s%s%s", color, msg, NC);
    }
};

#endif // SC_LOGCONSOLE_H
