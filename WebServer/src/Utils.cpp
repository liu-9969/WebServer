#include "../include/Utils.h"
#include "../../AsynLogSystem/include/Logging.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
//#define IP "127.0.0.1"
//#define IP "123.56.141.85"
#define IP "172.25.55.170"
using namespace swings;

int utils::creatListenFd(int port)
{
    //端口非法处理
    port = ((port <= 1024) || (port >= 65535)) ? 80 : port;

    //创建套接字
    int listenFd = 0;
    if((listenFd = ::socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK,0)) == -1){
        printf("utils::creatListenFd[%d],socket:%s\n",listenFd,strerror(errno));
        return -1;
    }

    //端口复用，避免“Address alread in use”
    int optval = 1;
    if(::setsockopt(listenFd,SOL_SOCKET,SO_REUSEADDR,(const void*)&optval,sizeof(int)) == -1){
        printf("utils::creatListenFd[%d],setsockopt:%s\n",listenFd,strerror(errno));
        return -1;
    }

    //绑定ip、端口
    struct sockaddr_in serverAddr;
    ::bzero(&serverAddr,sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons((unsigned short)port);
    inet_pton(AF_INET,IP,&serverAddr.sin_addr.s_addr);
  //  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(::bind(listenFd,(struct sockaddr*)&serverAddr,sizeof(serverAddr)) == -1){
        printf("utils::creatListenFd[%d],bind:%s\n",listenFd,strerror(errno));
        return -1;
    }

    //开始监听
    if(::listen(listenFd,LISTENQ) == -1){
        printf("utils::creatListenFd[%d],listenFd:%s\n",listenFd,strerror(errno));
        return -1;
    }

    //关闭无效描述符
    if(listenFd == -1){
        ::close(listenFd);
        return -1;
    }
    LOG<<CurrentThread::tid()<<"  WebServer Running "<<IP<<":"<<port; 
    return listenFd;
}


int utils::setNonBlock(int fd)
{
    int flag = ::fcntl(fd,F_GETFL,0); 
    if(flag == -1){
        printf("utils::setNonBlock[%d],listenFd:%s\n",fd,strerror(errno));
        return -1;
    }
    flag |= O_NONBLOCK;
    if(::fcntl(fd,F_SETFL,flag) == -1){
        printf("utils::setNonBlock[%d],listenFd:%s\n",fd,strerror(errno));
        return -1;
    }
    return 0;
}







