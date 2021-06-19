/**********************************************************************************************************************************************
 * 创建时间：2021-03-23
 * 修改时间：2021-03-24 23：0：0
 * 程序:     epoll反应堆模型（Libevent库核心思想）
 *
 *--------
 * 思路：|
 *--------
 *          1) epoll接口：
 *
 *                     1. 红黑树上fd对应的结构体中的联合体是文件描述符本身
 *
 *                     2. 当epoll_wait返回后，会对就绪事件进行分类处理
 *
 *
 *          2) epoll反应堆模型：
 *
 *                     1. 红黑树上fd对应的结构题中的联合体是我们自定义的结构体指针
 *
 *                     2. 当epoll_wait返回后，会直接去调用事件对应的【上树前已经确定好的】回调函数
 *
 *                     3. 监听可读事件-->数据到来-->触发事件-->epoll-wait返回-->
 *                        读完数据-->将该结点从树上摘下来-->设置为可写事件和可写回调函数-->重新挂上树-->一些收尾工作（这一行都在回调函数内做）
 *                       
 *                        监听可写事件-->数据到来-->触发事件-->epoll-wait返回-->
 *                        写完数据-->将该结点从树上摘下来-->设置为可读事件和可读回调函数-->重新挂上树-->一些收尾工作（这一行都在回调函数内做）
 *                        交替循环
 *
 *--------
 * 注意：|
 *--------
 *                      1) 用户需要自己开辟空间，存放my_events类型的数组。
 *                      2) 每次上树前都需要联合体epoll_data_t里的ptr指向一个my_event元素，
 *                         里边的元素要都初始化，尤其是回调函数都要对应好了
 *
 *
 *--------
 * 要点：|
 *--------
 *          1) 数组g_events[]的作用：
 *
 *                      1. 用来记录红黑树上现有的所有节点；以及曾经在树上，不过现在还未被覆盖的节点。
 *                      2. 因为要对树上的节点进行频繁的增加删除操作，而实际上就是对自定义的结构体进行修改；
 *                         而树又在内核中，所以我们要在用户区维护一个数组，来记录这些节点（事件）对应的信息。
 *
 *          2)
 *
 * ********************************************************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define MAX_EVENTS 1024
#define BUFLEN 4096
#define PORT 8080




/*自定义结构体,描述所关心、监听、已就绪文件描述符的相关信息*/
 struct my_events
 { 
    int    fd;                                          //要监听的文件描述符
    int    events;                                      //对应的事件
    void*  arg;                                         //泛型指针，指向它所属的结构体实例，即本身
    void  (*call_back)(int fd,int events,void* arg);    //回调函数，前三个元素是他的参数
    int    status;                                      //是否在树上（监听），在树上-1（监听），不在树上-0（不监听）
    char   buf[BUFLEN];                                 //存放从缓冲区读取的数据
    int    len;                                         //读取数据大小
    long   last_active;                                 //记录最后一次活动的时间
 };




/*定义的全局数组，变量，声明回调函数*/
int g_epfd;                                             //全局变量，树根，接收epoll_create返回值
struct my_events g_events[MAX_EVENTS+1];                //全局数组，在用户空间记录现打开的、曾经打开的，文件描述符
void recvdata(int fd,int events,void* arg);             //回调函数，读取缓冲区的数据 
void senddata(int fd,int events,void* arg);             //回调函数，往缓冲区写数据




/* 初始化结构体 my_events 变量的成员
 * 封装自定义的事件，当lfd所监听的事件事件发生时，要把cfd挂树上去
 * 流程：epoll_ctl调用 <-A- 传epoll_event结构体 <-- 把自己封装的结构体的地址赋给ptr 
 * *arg：自己封装的结构体成员arg就是指向他所在的这个结构体
 * 形参：结构体体变量的地址，文件描述符，回调函数，指向一个参数的指针（指向他自己）*/
void eventset(struct my_events* ev,int fd,void(*callback)(int fd,int events,void*arg),void*arg)
{
    ev->fd = fd;
    ev->events = 0;
    ev->arg = arg;
    ev->call_back = callback;
    ev->status = 0;
    ev->last_active = time(NULL);
}




/* 向红黑树上添加节点，与之前结构体不同的是，这个结构体的*ptr携带了大量信息
 * 形参：树根，eopllin/epollout，想加入节点所在的结构体的地址*/
void eventadd(int epfd,int how,struct my_events* ev)
{
    int op;
    struct epoll_event evt{0,{0}};
    evt.data.ptr = ev;
    evt.events = ev->events = how;  //events 和 ptr->events都赋值了
    if(ev->status == 1)
        op = EPOLL_CTL_MOD;
    else{
        op = EPOLL_CTL_ADD;
        ev->status = 1;
    }
    if(epoll_ctl(epfd,op,ev->fd,&evt) < 0)
        printf("eventadd failed[fd=%d],events[%d]\n",ev->fd,how);
    printf("eventadd ok[fd=%d],op%d],events[%0X]\n",ev->fd,op,how);
}




/* 从红黑树上摘下一个节点，
 * 形参：树根，想删除节点所在的结构体的地址*/
void eventdel(int epfd,struct my_events* ev)
{
    struct epoll_event evt{0,{0}};
    if(ev->status == 0)
        return;
    evt.data.ptr = ev;
    ev->status = 0;
    epoll_ctl(epfd,EPOLL_CTL_DEL,ev->fd,&evt);
}




/* 回调函数，当有客户端来来连接，lfd事件就绪，epoll_wait返回，调用这个函数，将cfd挂树上*/
 void acceptcon(int listen_fd,int events,void* arg)
{
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    int cfd,i;
    if((cfd = accept(listen_fd,(struct sockaddr*)&cliaddr,&len)) < 0){
        printf("");
    }
    do{
        for(i =0;i < MAX_EVENTS;i++)                          //和在select中找-1差不多
            if(g_events[i].status == 0)
                break;
        if(i == MAX_EVENTS){
            printf("");
            break;
        }
        int flag = 0;                                         
        if((flag = fcntl(listen_fd,F_SETFL,O_NONBLOCK)) < 0){ //设置为非阻塞属性
           printf("");
           break;
        }
        eventset(&g_events[i],cfd,recvdata,&g_events[i]);     //初始化和cfd对应的结构体成员
        eventadd(g_epfd,EPOLLIN,&g_events[i]);                //找到合适的位置后，就添加到树上，监听读事件
    }while(0);
    printf("new connec [%s:%d] time[%ld] pos[%d]",
           inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port),g_events[i].last_active,i);
}




/*cfd读事件响应的回调函数*/
/*传参：参数都来自我们自定义的结构体*/
void recvdata(int fd,int events,void* arg)
{
    struct my_events *ev = (struct my_events*)arg;
    int len = recv(fd,ev->buf,sizeof(ev->buf),0);
    eventdel(g_epfd,ev);                //recv完后，就从树上把这个节点拿掉
    if(len > 0){
        ev->len = len;
        ev->buf[len]='\0';
        printf("client[%d],%s\n",fd,ev->buf);
        eventset(ev,fd,senddata,ev);    //重新设置这个节点，回调函数改为senddata,事件改为监听可写
        eventadd(g_epfd,EPOLLOUT,ev);   //重新上树
    }
    else if(len == 0){
        close(fd);                      //ev->fd也可以，fd对应的实参就是ev->fd
        printf("[fd%d] pos[%ld],closed\n",fd,ev-g_events);//地址相减得到偏移位置元素地址
    }
    else{
        close(fd);
        printf("recv[fd=%d],error[%d]:%s]\n",fd,errno,strerror(errno));
    }
}




void senddata(int fd,int events,void* arg)
{
    struct my_events *ev = (struct my_events*)arg;
    int len = send(fd,ev->buf,ev->len,0);//直接将数据回写给用户，未做处理
    if(len > 0){
        printf("send[fd]=%d,[%d]%s\n",fd,len,ev->buf);
        eventdel(g_epfd,ev);             //摘下这个节点
        eventset(ev,fd,recvdata,ev);     //重新设置
        eventadd(g_epfd,EPOLLOUT,ev);    //上树
    }
    else{
        close(fd);
        eventdel(g_epfd,ev);             //摘下这个节点
        printf("send[fd=%d] error %s\n", fd, strerror(errno));
    }
}




/*创建socket,初始化listen_fd（上树），绑定ip、port，开始监听*/
void initListenSocket(short port)
{
    int lfd = socket(AF_INET,SOCK_STREAM,0);
    fcntl(lfd,F_SETFL,O_NONBLOCK);
    eventset(&g_events[MAX_EVENTS],lfd,acceptcon,&g_events[MAX_EVENTS]);//第一个参数是用来设置的，最后一个参数就是要这个地址
    eventadd(g_epfd,EPOLLIN,&g_events[MAX_EVENTS]);//最后一个参数就是要个地址，因为这时候已经set完了，我要确认它当前不在树上

    struct sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(lfd,(struct sockaddr*)&addr,sizeof(addr));
    listen(lfd,20);
}




int main(int argc,char* argv[])
{
//用户指定端口，否则默认
    unsigned short port = PORT;
    if(argc == 2)
    port = atoi(argv[1]);   

//创建红黑树
    g_epfd = epoll_create(MAX_EVENTS+1);
    if(g_epfd <= 0)
    printf("create efd in %s err %s\n", __func__, strerror(errno));

//创建套接字、绑定、监听、listen_fd上树
    initListenSocket(port);
    
//保存已经就绪事件的数组，
    struct epoll_event evts[MAX_EVENTS+1];
    printf("server running [port%d]\n",port);

//阻塞等待是否有事件发生，是则返回
    int checkpos = 0, i;
    while(1)
    {
        /* 超时验证，每次测试100个链接，不测试listenfd 当客户端60秒内没有和服务器通信，则关闭此客户端链接 */
        long now = time(NULL);                                          //当前时间
        for (i = 0; i < 100; i++, checkpos++) {                         //一次循环检测100个。 使用checkpos控制检测对象
            if (checkpos == MAX_EVENTS)
                checkpos = 0;
            if (g_events[checkpos].status != 1)                         //不在红黑树 g_efd 上
                continue;

            long duration = now - g_events[checkpos].last_active;       //客户端不活跃的世间

            if (duration >= 60) {
                close(g_events[checkpos].fd);                           //关闭与该客户端链接
                printf("[fd=%d] timeout\n", g_events[checkpos].fd);
                eventdel(g_epfd, &g_events[checkpos]);                   //将该客户端 从红黑树 g_efd移除
            }
        }



        int nfd = epoll_wait(g_epfd,evts,MAX_EVENTS+1,1000);
        if(nfd < 0){
            printf("epoll_wait error,exit\n");
            break;
        }
        for(int i = 0;i < nfd;i++)
        {
            struct my_events *ev = (struct my_events*)evts[i].data.ptr;     //使用自定义结构体指针接收ptr
            if((evts[i].events & EPOLLIN) && (ev->events & EPOLLIN)){       //读事件就绪
                ev->call_back(ev->fd,ev->events,ev->arg);
            }
            if((evts[i].events & EPOLLOUT) && (ev->events & EPOLLOUT)){     //写事件就绪
                ev->call_back(ev->fd,ev->events,ev->arg);
            }
            else{
                 /*这里可以做很多很多其他的工作，例如定时清除没读完的不要的数据
                   也可以做点和数据库有关的设置
                   玩大点你在这里搞搞分布式的代码也可以
                 */
            }
        }
    }


/*退出前释放所有资源*/
    return 0;
}









