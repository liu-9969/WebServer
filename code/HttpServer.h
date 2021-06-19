#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_


#include <memory>
#include <mutex>

#define TIMEOUT -1 // epoll_wait超时时间，-1不超时
#define CONNECT_TIMEOUT 500 // 连接默认超时时间
#define NUM_WORKERS 4 //线程池大小

namespace swings
{
    class HttpRequest;
    class Epoll;
    class ThreadPool;
    class TimerManager;

    class HttpServer
    {
        public:
            HttpServer(int port,int numThread);
            ~HttpServer();
            void run(); // 启动http服务器
        private:
            void _acceptConnection(); // 接受新连接
            void _closeConnection( HttpRequest* request );//关闭连接
            void _doRequest( HttpRequest* request ); // 处理http请求，这个函数由线程池调用
            void _doResponse( HttpRequest* request ); // 响应http请求 

        private:
            using ListenRequestPtr = std::unique_ptr<HttpRequest>;
            using EpollPtr = std::unique_ptr<Epoll>;
            using ThreadpoolPtr = std::shared_ptr<ThreadPool>;
            using TimerManagerPtr = std::unique_ptr<TimerManager>;

            int port_; // 监听端口
            int listenFd_; // 监听fd
            ListenRequestPtr listenRequest_; // 监听套接字的HttpRequest的实例
            EpollPtr epoll_; // Epoll实例
            ThreadpoolPtr threadPool_; // 线程池实例
            TimerManagerPtr timerManager_; // 定时管理器实例
    };
}

#endif
