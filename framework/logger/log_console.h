#ifndef SC_LOGCONSOLE_H
#define SC_LOGCONSOLE_H

#include "log.h"

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
    inline virtual void SendMessage(const char* msg) {
        fprintf(stderr, "%s", msg);
    }
};

#endif // SC_LOGCONSOLE_H
