#ifndef _EPOLL_H_
#define _EPOLL_H_

#include <vector>
#include <functional>
#include <sys/epoll.h>
#include <memory>

#define MAXEVENTS 1024

namespace swings
{
    class HttpRequest;
    class ThreadPool;
    class Epoll
    {
        public:
            using NewConnectionCallBack = std::function<void()>;
            using CloseConnectionCallBack = std::function<void(HttpRequest*)>;
            using HandleRequestCallBack = std::function<void(HttpRequest*)>;
            using HandleResponseCallBack = std::function<void(HttpRequest*)>;
            // 一个可调用对象（可以是函数对象、函数指针、函数引用、成员函数指针、数据成员指针）
            
            Epoll();
            ~Epoll();

            int add( int fd,HttpRequest* request,int events ); // 注册新描述符--EPOLL_CTL_ADD
            int mod( int fd,HttpRequest* request,int events ); // 修改描述符--  EPOLL_CTL_MOD
            int del( int fd,HttpRequest* request,int events ); // 删除描述符--  EPOLL_CTL_DEL
            int wait( int timeoutMS); // 等待事件发生，返回活跃的描述符数量
            void handleEvent( int listenFd, std::shared_ptr<ThreadPool>& threadpool, int eventsNum); // 调用事件处理函数

            void setOnConnection( const NewConnectionCallBack& cb ) {onConnection_ = cb; } // 设置新连接回调函数
            void setOnCloseConnection( const CloseConnectionCallBack& cb ) {onCloseConnection_ = cb;} // 设置关闭连接回调函数
            void setOnRequest( const HandleRequestCallBack& cb ) {onRequest_ = cb;}   // 设置解析请求回调函数
            void setOnResponse( const HandleResponseCallBack& cb ) {onResponse_ = cb;} // 设置封装响应回调函数


        private:
            using eventList = std::vector<struct epoll_event>;
            int epollFd_;
            eventList events_;
            NewConnectionCallBack onConnection_;
            CloseConnectionCallBack onCloseConnection_;
            HandleRequestCallBack onRequest_;
            HandleResponseCallBack onResponse_;
    };
}

#endif

