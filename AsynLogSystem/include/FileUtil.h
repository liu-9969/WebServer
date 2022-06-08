#ifndef _FILEUTIL_H_
#define _FILEUTIL_H_

#include "noncopyable.h"
#include <string>
#include <stdio.h>

class AppendFile:noncopyable
{
public:
    explicit AppendFile(std::string filename );
    ~AppendFile();
    void append(const char* logline,const size_t len);//对write()的封装
    void flush();//对fflush()的封装

private:
    size_t write(const char* logline,size_t len);//对fwrite的封装，往磁盘写。
    FILE* fp_;//文件指针
    char buffer_[64 * 1024];//文件流缓冲
};

#endif

