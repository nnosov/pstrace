#include "log.h"

#include <time.h>

#define LOG_BUFF_SIZE       (1024*1024)


const char * const SC_LogBase::mSeverityStrings[] = {
		" [DEBUG]  : ",
        " [INFO]   : ",
        " [WARNING]: ",
        " [ERROR]  : "
};

char         SC_LogBase::mString[LOG_BUFF_SIZE];
uint32_t     SC_LogBase::mStringSize = sizeof(SC_LogBase::mString);

SC_LogBase::SC_LogBase(uint32_t id, const char* source) : mID(id)
{
    mStringLen = 0;
    mCurrentSeverity = SEVERITY_DEBUG;
    if(source) {
        mSource = source;
    }
}

SC_LogBase::~SC_LogBase()
{
    Close();
}

void SC_LogBase::FormatString(const char* fmt, va_list args)
{
    mStringLen += vsnprintf(mString + mStringLen, mStringSize - mStringLen, fmt, args);
}

void SC_LogBase::FormatPrefix(SC_LogSeverity severity)
{
    time_t rawTime;
    struct tm * timeinfo;
    time(&rawTime);
    timeinfo = gmtime(&rawTime);

    mStringLen += strftime(mString, sizeof(mString), "%d-%m-%Y %H:%M:%S", timeinfo);
    strncpy(mString + mStringLen, mSeverityStrings[(int)severity], strlen(mSeverityStrings[(int)severity]));
    mStringLen += strlen(mSeverityStrings[(int)severity]);
}

void SC_LogBase::FormatPostfix()
{
    if((mStringLen + 2) < mStringSize) {
        mString[mStringLen++] = '\n';
        mString[mStringLen++] = 0;
    }
}
