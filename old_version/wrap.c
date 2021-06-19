/**********************************************************
 * 源文件：重新封装一些系统调用
 * 创建时间：2021-03-14
 * 修改时间：2021-03-14
 *
 * ********************************************************/

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
void err_exit(const char* s)
{
    perror(s);
    exit(-1);
}


int Socket(int domain,int type,int protocol)
{
    int n = socket(domain,type,protocol); 
    if (n < 0)
       err_exit("socket error");
    return  n;
}



int Bind(int sockfd,const struct sockaddr*addr,socklen_t addrlen)
{
    int n;
    n = bind(sockfd,addr,addrlen);
    if(n < 0)
        err_exit("bind error");
    return n;
}


int Listen(int sockfd,int backlog)
{
    int n;
    n = listen(sockfd,backlog);
    if(n < 0 )
        err_exit("listen error");
    return n;
}


int Connect(int sockfd,const struct sockaddr* addr,socklen_t addrlen)
{
    int n;
    n = connect(sockfd,addr,addrlen);
    if(n < 0)
        err_exit("connect error");
    return n;
}

/*EINTR:慢速系统调用                                           */
/*ECONNABORTED: 非阻塞操作文件返回                               */
int Accept(int sockfd,struct sockaddr* addr,socklen_t* addrlen)
{
    int n;
    again:
    n = accept(sockfd,addr,addrlen);
    if(n < 0){
        if(errno == EINTR || errno == ECONNABORTED)
            goto again;
        else    
        err_exit("accept error");
    }
    return n;
}


int Close(int fd)
{
    int n;
    n = close(fd);
    if(n < 0)
        err_exit("close error");
    return n;
}


ssize_t Read(int fildes,void* buf,size_t nbyte)
{
    ssize_t n;
    again:
    n = read(fildes,buf,nbyte);
    if(n == -1){
        if(errno == EINTR)
            goto again;
        else
            err_exit("read error");
    }
    return n;
}


ssize_t Write(int fildes,void* buf,size_t nbyte)
{
    ssize_t n;
    again:
    n = write(fildes,buf,nbyte);
    if(n == -1){
        if(errno == EINTR)
            goto again;
        else
            err_exit("read error");
    }
    return n;
}

//指定读取多少字节，保证的
ssize_t Readn(int fd,void* vptr,size_t n)
{
    char* ptr = (char*)vptr;
    int leftn = n;//记录未读取的字节数
    ssize_t readn;
    while(leftn > 0)
    {
        readn = read(fd,ptr,n);
        if(readn < 0){
            if(readn == EINTR)
                readn =0;
            else
                return -1;
        }
        else if(readn == 0)
            break;
        else{
            leftn = leftn - readn;
            ptr = ptr + readn;
        }
    }
    return n - leftn;
}


ssize_t Writen(int fd,const void* vptr,size_t n)
{
    const char* ptr = (char*)vptr;
    size_t writen;
    int leftn = n;
    while(leftn > 0)
    {
        writen = write(fd,ptr,n);
        if(writen < 0){
            if(errno == EINTR)
                writen = 0;
            else
                return -1;
        }
        else if(writen > 0){
            leftn = leftn - writen;
            ptr = ptr + writen;
        }
        else 
            break;   
    }
    return n - leftn;
}

/*********************************************************************
 * 功能： 负责从某些文件读到全局的一个缓冲区
 * 特点： 多次调用这个函数，整个if分支基本只会被调用一次
 *
 * 全局变量：
 * read_cnt:用来标记缓冲区里还有多少字节未被c取走
 * read_ptr:用来指向缓冲区里，第一个未被读取的字符，
 *          或者说是下一次将要读取的字符
 *
 *
 * ******************************************************************/

static ssize_t my_read(int fd,char* ptr)
{
    static int read_cnt;
    static char read_buf[100];
    static char* read_ptr;

    again:
    if(read_cnt <= 0)
    {
        read_cnt = read(fd,read_buf,sizeof(read_buf));
        if(read_cnt < 0){
            if(errno == EINTR)
                goto again;
            return -1;
        }
        else if(read_cnt == 0)
            return 0;
        else
            read_ptr = read_buf;
    }
    read_cnt--;
    *ptr = *read_ptr++;
    return 1;
}

/********************************************************************
 * 功能：负责从全局缓冲区读到传入的缓冲区
 * 特点：循环调用my_read，一个字符一个字符的读取
 *
 * 缺点：如果maxlen过大，
 *
 *
 *
 *
 ********************************************************************/
ssize_t Readline(int fd,void* vptr,size_t maxlen)
{
    ssize_t cn;
    char c;
    char* ptr = (char*)vptr;
    size_t n;
    for(n =1;n < maxlen;n++)
    {
        cn = my_read(fd,&c);
        if(cn == 1){
            *ptr = c;
            if(c == '\n')
                break;
        }
        else if(cn == 0){
            *ptr = 0;
            return n -1;
        }
        else
            return -1;
    }
    *ptr = 0;//把最后的\n换为0
    return n;
}










