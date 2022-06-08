/*2021-5-10*/



#ifndef _UTILS_H_
#define _UTILS_H_
#define LISTENQ 1024//监听队列长度，操作系统默认SOMAXCONN

namespace swings
{
     namespace utils
     {
        int creatListenFd(int port);    //创建监听描述符
        int setNonBlock(int fd);        //设置非阻塞模式
     }
}

#endif



