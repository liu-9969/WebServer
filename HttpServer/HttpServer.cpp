#include "HttpServer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "../include/ThreadPool.h"
#include "../include/Epoll.h"
#include "../include/Timer.h"
#include "../include/Utils.h"
#include "../include/MysqlPool.h"
//#include "../../BackEnd/include/StudentService.h"
#include "../../AsynLogSystem/include/Logging.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <string>
using std::string;
using namespace swings;

HttpServer::HttpServer(int port,int numThread)
    :port_(port),
    listenFd_ (utils::creatListenFd(port_)),
    listenRequest_(new HttpRequest(listenFd_)),
    epoll_(new Epoll()),
    threadPool_(new ThreadPool(numThread)),
    timerManager_(new TimerManager()),
    mysqlPool_(Mysql_pool::GetInstance())
{
    assert(listenFd_ >= 0);
    mysqlPool_->init("rm-2zefx74b3w67he734ko.mysql.rds.aliyuncs.com","root","root@liu123","course_select_sys",3306,1000);
}

HttpServer::~HttpServer()
{}



//给Epoll的那几个成员函数(可调用实体)注册好回调函数。
void HttpServer::run()
{
    //注册监听套接字到epoll,(可读事件，ET)
    /* 绑定类成员函数要有this/或者一个对象，调用时传入参数要提前写std::placeholders::_1占位符*/
    epoll_->add(listenFd_,listenRequest_.get(),(EPOLLIN | EPOLLET));
    epoll_->setOnConnection(std::bind(&HttpServer::_acceptConnection,this));
    epoll_->setOnCloseConnection(std::bind(&HttpServer::_closeConnection,this,std::placeholders::_1));
    epoll_->setOnRequest(std::bind(&HttpServer::_doRequest,this,std::placeholders::_1));
    epoll_->setOnResponse(std::bind(&HttpServer::_doResponse,this,std::placeholders::_1));

    //事件循环 Event_Loop
    LOG<<CurrentThread::tid()<<"  EventLoop start";
    while(1)
    {
        int timeMS = timerManager_->getNextExpireTime();//获得最快要超时的那个计时器的剩余时间，作为epoll_wait的超时时间
        int eventsNum = epoll_->wait(timeMS);
        if(eventsNum > 0)
        epoll_->handleEvent(listenFd_,threadPool_,eventsNum);
        timerManager_->handleTimer();//处理优先队列，超时、已关闭的timer给delete掉，释放资源
    }
}



//接受新连接
//要放在while（1）循环里，否则，因为listenFd是ET模式，当同时有多个连接到来时只会触发一次，这样accept只能取一个连接
//其他连接就延后处理了
void HttpServer:: _acceptConnection()
{
    while(1)
    {
        int acceptFd = ::accept4(listenFd_,nullptr,nullptr,SOCK_NONBLOCK | SOCK_CLOEXEC);
        if(acceptFd == -1)
        {
            if(errno == EAGAIN)
                break;
            //printf("[HttpServer::__acceptConnection] accept : %s\n", strerror(errno));
            LOG<<CurrentThread::tid()<<"  accept error "<<strerror(errno); 
            break;
        }
        HttpRequest* request = new HttpRequest(acceptFd);
        // 给request实例安一个定时器
        timerManager_->addTimer(request,CONNECT_TIMEOUT,std::bind(&HttpServer::_closeConnection, this, request));
        //注册连接套接字到epool（保证任意时刻都只被一个线程处理，避免在处理这个socket时，它又就绪了，又给他分配线程）
        epoll_->add(acceptFd,request,(EPOLLIN | EPOLLONESHOT));//挂树上
        LOG<<CurrentThread::tid()<<"  New Collection"; 
    }
}


//关闭连接 
void HttpServer::_closeConnection( HttpRequest* request )
{
    int fd = request->fd();
    if(request->isWorking())
        return;
    timerManager_->delTimer(request);//request的定时器开关关闭
    epoll_->del(fd,request,0);//从树上摘下

    //释放该套接字占用的HttpRequest资源,在析构函数中close(fd);
    delete request;
    request = nullptr;//避免悬垂指针，该指针指向一块非法内存
}



//LT
void HttpServer::_doRequest( HttpRequest* request)
{
    LOG<<CurrentThread::tid()<<"  Do Resquest"; 
    timerManager_->delTimer(request);
    assert(request != nullptr);
    int fd = request->fd();

    int readError;
    int nRead = request->read(&readError);

    //read返回0表示客户端断开连接
    if(nRead == 0)
    {
        request->setNoWorking(); // 释放这个request前，必须保证working是关闭的
        _closeConnection(request);
        LOG<<CurrentThread::tid()<<"  read 0断开连接"; 
        return;
    }

    //非EAGAIN错误，断开连接
    if(nRead < 0 && (readError != EAGAIN))
    {
        LOG<<CurrentThread::tid()<<"  非EAGAIN错误，断开连接";
        request->setNoWorking();
        _closeConnection(request);
        return;
    }

    //EAGAIN错误,释放线程使用权。因为不想发数据，所以不监听可写。
    if(nRead < 0 && (readError == EAGAIN))
    {
        //因为是EPOLLONESHOT，所以第二次监听它必须重新配置文件描述符上对应的事件;
        epoll_->mod(fd,request,(EPOLLIN | EPOLLONESHOT));        
        request->setNoWorking();
        timerManager_->addTimer( request,CONNECT_TIMEOUT,std::bind(&HttpServer::_closeConnection,this,request));
        return;
    }

    //read返回值大于0,但是解析错误
    if(!(request->parseRequest()))
    {
        int writeError;
        //封装400报文
        HttpResponse response(400,"","",false);
        //拷贝到request维护的outBuffer缓冲区
        request->appendOutBuffer(response.makeResponse());
        //发送400报文
        request->write(&writeError);
        request->setNoWorking();
        _closeConnection(request);
        LOG<<CurrentThread::tid()<<"  解析错误断开连接"; 
        return;
    }

    //read返回值大于0，并且解析完成
    if(request->parseFinish())
    {
        // 解析完成，加入业务队列
        threadPool_->Curd_pushJob(std::bind(&HttpServer::_curdJob,this,request));
        // 业务线程while(1)判队列是否空，进行处理，不需要通知。      
    }

}

// 业务函数是返回Buffer还是string呢,IO线程还是只让他负责读写吧
// EPOLLONRSHORT：注册这个事件可以保证，在下次注册这个事件前，即便有事件到来，epoll也不会通知，在哪个线程注册无所谓
// 我也不要求，一个事件的处理必须在一个线程完成(某个线程读socket + 某个线程处理业务 + 某个线程写回socket )
// 我要求的是一个事件处理过程中，epoll不可以再通知我了，否则就是多个线程处理一个SOCKET,会分配多个缓存的。
// 业务处理应该在这里大做文章 ,好难，业务和webserver分离在目录上就不好分离啊，响应怎么可能不和业务挂钩呢。

void HttpServer::_curdJob( HttpRequest* request )
{
    int fd = request->fd();
    HttpResponse reponse(200,request->getPath(),request->getQuery(),request->keepAlive());
    request->appendOutBuffer(reponse.makeResponse());
    epoll_->mod(fd,request,(EPOLLIN | EPOLLOUT | EPOLLONESHOT ));
}


void HttpServer::_doResponse( HttpRequest* request )
{
    LOG<<CurrentThread::tid()<<"  Do Response";
    timerManager_->delTimer(request);
    assert(request != nullptr);
    int fd = request->fd();
    int toWrite = request->writeableBytes();//outBuffer缓冲区可读数据

    if(toWrite == 0)
    {
        epoll_->mod(fd,request,(EPOLLIN | EPOLLONESHOT));
        timerManager_->addTimer(request,CONNECT_TIMEOUT,std::bind(&HttpServer::_closeConnection,this,request));
        request->setNoWorking();
        return;
    }

    int writeError;
    int ret = request->write(&writeError);
    
    //EAGAIN错误，没有写入成功，那就继续监听，一有机会就立即写
    if(ret < 0 && (writeError == EAGAIN))
    {
        epoll_->mod(fd,request,(EPOLLIN | EPOLLOUT | EPOLLONESHOT ));
        return;
    }

    //非EAGAIN错误，断开连接
    if(ret < 0 && (writeError != EAGAIN))
    {
        request->setNoWorking();
        _closeConnection(request);
        LOG<<CurrentThread::tid()<<"  非EAGAIN错误断开连接";  
        return;
    }

    //刚好写完
    if(ret == toWrite)
    {
        if(request->keepAlive())
        {
            request->resetParse();//重置解析结果
            epoll_->mod(fd,request,(EPOLLIN | EPOLLONESHOT));
            request->setNoWorking();
            timerManager_->addTimer(request,CONNECT_TIMEOUT,std::bind(&HttpServer::_closeConnection,this,request));
        }
        else
        {
            request->setNoWorking();
            _closeConnection(request);
            LOG<<CurrentThread::tid()<<"  短链接断开连接";
        }
        return;
    }

    //没写完的情况,还要继续监听可写，有机会就立即写入
    epoll_ -> mod(fd, request, (EPOLLIN | EPOLLOUT | EPOLLONESHOT));
    request -> setNoWorking();
    timerManager_ -> addTimer(request, CONNECT_TIMEOUT, std::bind(&HttpServer::_closeConnection, this, request));
    return;

}































