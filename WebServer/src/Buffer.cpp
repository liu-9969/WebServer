#include "Buffer.h"
#include <cstring> // perror
#include <iostream>
#include <unistd.h> // write
#include <sys/uio.h> // readv
#include "../../AsynLogSystem/include/Logging.h"
using namespace swings;

ssize_t Buffer::readFd(int fd, int* saveErrno)//散布读
{
    // 保证一次读到足够多的数据
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writeable = writeableBytes();
    vec[0].iov_base = _begin() + writerIndex_;
    vec[0].iov_len = writeable;
    vec[1].iov_base = extrabuf; 
    vec[1].iov_len = sizeof(extrabuf);
    //const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, 2);
    if(n < 0) {
        //printf("[Buffer:readFd]fd = %d readv : %s\n", fd, strerror(errno));
        LOG<<CurrentThread::tid()<<"Buffer:readFd:"<<fd<<strerror(errno);
        *saveErrno = errno;
    } 
    else if(static_cast<size_t>(n) <= writeable)
        writerIndex_ += n;
    else{
        writerIndex_ = buffer_.size();//writerIndex移动到最后
        append(extrabuf, n - writeable);
    }

    return n;
}
    //如果buffer缓冲区未装下，把extrabuf中的内容添加进buffer
    //readv无法使vector进行扩容，当n太大时，会把vector填满并且剩余字节读到extrabuf，然后我们已经准备好成员函数makespace进行扩容，
    //所以我们调用append()，把extrabuf里的数据拷贝到vector。
    

ssize_t Buffer::writeFd(int fd, int* savedErrno)//聚集写
{
    size_t nLeft = readableBytes();
    char* bufPtr = _begin() + readerIndex_;
    ssize_t n;
    if((n = ::write(fd, bufPtr, nLeft)) <= 0) {
        if(n < 0 && n == EINTR)
            return 0;
        else {
            printf("[Buffer:writeFd]fd = %d write : %s\n", fd, strerror(errno));
            *savedErrno = errno;
            return -1;
        }
    } else {
        readerIndex_ += n;
        return n;
    }
}
