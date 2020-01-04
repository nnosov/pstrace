#ifndef LOG_H_
#define LOG_H_

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include "mutex.h"

using std::string;

// Log message severity
enum SC_LogSeverity {
    SEVERITY_DEBUG = 0,
    SEVERITY_INFO,
    SEVERITY_WARNING,
    SEVERITY_ERROR,
    //----------------
    SEVERITY_MAX
};


class SC_LogBase
{
protected:

    SC_LogBase(uint32_t id, const char* source);

public:

    virtual ~SC_LogBase();

    //variable argument number logging
    inline virtual void Log(SC_LogSeverity severity, const char* fmt, ...)
    {
        if (severity < mCurrentSeverity)
            return;

        mLock.Lock();

        if(!IsOpened())
        {
            //print header if present
            if(Open() && mHeader.size())
            {
                SendMessage((char*)mHeader.c_str());
            }
        }

        mStringLen = 0;
        FormatPrefix(severity);
        va_list args;
        va_start(args, fmt);
        FormatString(fmt, args);
        va_end(args);
        FormatPostfix();
        SendMessage(mString);

        mLock.Unlock();
    }

    //open source for send messages
    virtual bool Open() = 0;

    //close source
    virtual void Close(){}

    //whether source already opened
    virtual bool IsOpened() = 0;

    //returns source path
    string& GetSource()
    {
        return mSource;
    }

    //assign source path
    inline void SetSource(const char* source)
    {
        Close();
        mSource = source;
    }

    void SetSource(string& source)
    {
        SetSource(source.c_str());
    }

    //assign Log header
    inline void SetHeader(const char* header)
    {
        mHeader = string(header) + '\n';
    }

    //return Log ID
    inline uint32_t GetID() const
    {
        return mID;
    }

    //set Log ID
    inline void SetID(uint32_t id)
    {
        mID = id;
    }

    //return current severity threshold applied to the messages
    inline SC_LogSeverity GetCurrentSeverity() const
    {
        return mCurrentSeverity;
    }

    //set severity threshold to be applied to the messages
    inline void SetCurrentSeverity(SC_LogSeverity severity)
    {
        mCurrentSeverity = severity;
    }

protected:

    //actually sends message to the source
    virtual void SendMessage(const char* msg) = 0;

    //format message prefix
    virtual void FormatPrefix(SC_LogSeverity severity);

    //format message body
    virtual void FormatString(const char* fmt, va_list args);

    //format message postfix
    virtual void FormatPostfix();

protected:

    uint32_t                    mID;                //log ID, associated with source
    string                      mSource;            //source for store log messages (file, IP address or sometsing else)
    string                      mHeader;            //header of the Log
    static char                 mString[];          //formatted string to be printed
    static uint32_t             mStringSize;        //size of the string buffer
    uint32_t                    mStringLen;         //length of the string
    SC_LogSeverity            	mCurrentSeverity;   //maximum severity value to be logged
    static const char * const 	mSeverityStrings[];
    SC_Mutex                 	mLock;
};

#endif /* LOG_H_ */
