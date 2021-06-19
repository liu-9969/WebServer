#include "Epoll.h"
#include "HttpRequest.h"
#include "ThreadPool.h"

#include <string.h>
#include <unistd.h>
using namespace swings;

Epoll::Epoll()
    :epollFd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(MAXEVENTS)
{
    assert(epollFd_ >= 0);
}

Epoll::~Epoll()
{
    ::close(epollFd_);
}

int Epoll::add( int fd, HttpRequest* request, int events)
{
    struct epoll_event event;
    event.events = events;
    event.data.ptr = (void*)request;
    int ret  = ::epoll_ctl(epollFd_,EPOLL_CTL_ADD,fd,&event);
    return ret;
}

int Epoll::mod( int fd, HttpRequest* request, int events)
{
    struct epoll_event event;
    event.events = events;
    event.data.ptr = (void*)request;
    int ret = ::epoll_ctl(epollFd_,EPOLL_CTL_MOD,fd,&event);
    return ret;
}

int Epoll::del( int fd, HttpRequest* request, int events)
{
    struct epoll_event event;
    event.events = events;
    event.data.ptr = (void*)request;
    int ret = ::epoll_ctl(epollFd_,EPOLL_CTL_DEL,fd,&event);
    return ret;
}

int Epoll::wait( int timeoutMS)
{
    //printf("epollFd:%d\n",epollFd_);
    int eventsNum = epoll_wait(epollFd_,&*events_.begin(),static_cast<int>(events_.size()),timeoutMS); // 不可以从迭代器强转为struct epoll_event* 
    if(eventsNum == 0){}
        //printf("[EPOLL::wait] nothing happen,epoll timeout\n");
    if(eventsNum < 0)
        printf("[Epoll::wait] epoll : %s\n", strerror(errno));
    return eventsNum;
}


void Epoll::handleEvent( int listenFd, std::shared_ptr<ThreadPool> &threadpool, int eventsNum )
{
    assert(eventsNum > 0);
    for(int i = 0; i < eventsNum; i++)
    {
        HttpRequest* request = (HttpRequest*)events_[i].data.ptr;
        if(request->fd() == listenFd){
            //printf("1.fd上树\n");
            onConnection_();//新连接回调函数
        }
        else
        {
            if((events_[i].events & EPOLLERR)||//排除错误事件
               (events_[i].events & EPOLLHUP))
               //(!(events_[i].events & EPOLLIN)&&(!(events_[i].events & EPOLLOUT))))
            {
                printf("错无事件\n");
                request->setNoWorking();//出错关闭连接
                onCloseConnection_(request);
            }
            else if(events_[i].events & EPOLLIN)
            {
                request->setWorking();
                threadpool->pushJob(std::bind(onRequest_,request)); // 预先给可调用实体的参数绑定已有的变量
            }
            else if(events_[i].events & EPOLLOUT)
            {
                request->setWorking();
                threadpool->pushJob(std::bind(onResponse_,request));
            }
            else
                printf("[Epoll::handleEvent] unexpected event\n");
        }
    }
    return ;
}



/*epoll原型
int epoll_create(int size);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event);   //哪颗树，增删改，哪个套接字，事件结构体                                                                   
int epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout);

typedef union epoll_data {
    void* ptr;
    int          fd;
    uint32_t     u32;
    uint64_t     u64;
} epoll_data_t;

struct epoll_event {
    uint32_t     events;      // Epoll events                                                                                                 
    epoll_data_t data;        // User data variable                                                                                           
};

*/
























