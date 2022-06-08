/* 对AppendFile的封装,增加了对磁盘写n次才刷一次的功能 */
#ifndef _LOGFILE_H_
#define _LOGFILE_H_

#include <memory>
#include <string>
#include "noncopyable.h"
#include "FileUtil.h"
#include "MutexLock.h"

class LogFile:noncopyable
{
public:
    LogFile(const std::string &basename,int flushEveryN = 1024);
    ~LogFile();
    void append(const char* logline,int len);
    void flush();

private:
    void append_unlocked(const char* logline,int len);
    
    const std::string basename_;//日志文件名
    const int flushEveryN_;//往磁盘写的周期
    int count_;//append的次数
    std::unique_ptr<MutexLock> mutex_;//互斥锁
    std::unique_ptr<AppendFile> file_;//最靠近磁盘的操作

};
#endif




