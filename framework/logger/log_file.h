#ifndef LOG_FILE_H_
#define LOG_FILE_H_

//framework
#include "log.h"
#include <stdio.h>
#include <sys/types.h>

using std::string;

//=============================================================================================
// SWI Logger to file logging class declaration
//=============================================================================================
class SC_LogFile : public SC_LogBase
{
public:

    SC_LogFile(uint32_t id, const char* source) : SC_LogBase(id, source)
    {
        mFile = 0;
        //2GByte by default
        mMaxFileSize = 0x0000000080000000ULL;
        mBytesCount = 0;
    }

    //open file for writing
    inline virtual bool Open()
    {
        if(!mFile) {
            // extract log and path names
            uint32_t namelen = mSource.size();
            if(namelen) {
                const char* ptr = strrchr(mSource.c_str(), '/');
                uint32_t pathlen = 0;
                if(ptr) {
                    pathlen = ptr - mSource.c_str() + 1;
                    namelen -= pathlen + 1;
                }

                mLogPath.append(mSource.c_str(), pathlen);

                mLogName.append((mSource.c_str() + pathlen), namelen);

                //open the file finally
                mFile = fopen(mSource.c_str(), "a+");
            }
        }

        return (mFile != 0);
    }

    //close file opened
    inline virtual void Close()
    {
        if(mFile) {
            fclose(mFile);
        }

        mFile = 0;
        mBytesCount = 0;
    }

    //whether file already opened or not
    inline virtual bool IsOpened()
    {
        return (mFile != 0);
    }

    inline void SetMaxFileSize(uint64_t size)
    {
        mMaxFileSize = size;
    }

    inline uint64_t GetMaxFileSize()
    {
        return mMaxFileSize;
    }

    inline FILE* GetLogFD()
    {
        return mFile;
    }

protected:

    inline virtual void SendMessage(const char* msg)
    {
        if(mFile && mBytesCount < mMaxFileSize) {
            if(fputs(msg, mFile) >= 0) {
            	fflush(mFile);
            	mBytesCount += mStringLen;
            } else {
            	fclose(mFile);
            	mFile = 0;
            }
        }
    }

protected:

    string              mLogPath;       //path to the log file
    string              mLogName;       //name of the log file
    FILE*               mFile;          //log file descriptor
    uint64_t            mMaxFileSize;   //maximum number bytes allowed to be written to the file
    uint64_t            mBytesCount;    //number of bytes actually written
};

#endif /* LOG_FILE_H_ */
