#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include "wrap.h"
#define MAXSIZE 1000
#define SERV_IP "172.16.38.175"

//创建、绑定listen_fd
int startup(int port)
{
    printf("port:%d\n",port);
    struct sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET,SERV_IP,&addr.sin_addr.s_addr);
    int listen_fd = Socket(AF_INET,SOCK_STREAM,0);
    int opt = 1;
    setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    Bind(listen_fd,(struct sockaddr*)&addr,sizeof(addr));
    Listen(listen_fd,64);
    return listen_fd;
}





/************************************************************************
 * 说明：
 * 以下3个函数是关于字符转码的东西，
 * 就是浏览器URL中文件名如果是类似中文这种方块的字体，要转成%1a%2d
 * 前端做页面研究的东西，会用就行
 * 不需要明白逻辑是啥，保存好，用的时候直接拿过来用就ok
 **********************************************************************/
// 16进制数转化为10进制
int hexit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}

/*
 *  这里的内容是处理%20之类的东西！是"解码"过程。
 *  %20 URL编码中的‘ ’(space)
 *  %21 '!' %22 '"' %23 '#' %24 '$'
 *  %25 '%' %26 '&' %27 ''' %28 '('......
 *  相关知识html中的‘ ’(space)是&nbsp
 */
void encode_str(char* to, int tosize, const char* from)
{
    int tolen;

    for (tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from) {    
        if (isalnum(*from) || strchr("/_.-~", *from) != (char*)0) {      
            *to = *from;
            ++to;
            ++tolen;
        } else {
            sprintf(to, "%%%02x", (int) *from & 0xff);
            to += 3;
            tolen += 3;
        }
    }
    *to = '\0';
}

void decode_str(char *to, char *from)
{
    for ( ; *from != '\0'; ++to, ++from  ) {     
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {       
            *to = hexit(from[1])*16 + hexit(from[2]);
            from += 2;                      
        } else {
            *to = *from;
        }
    }
    *to = '\0';
}





//读取http报文的每一行，\r\n不读入缓冲区,结尾手动添加\0作为字符串结尾标记
int get_line(int cfd,char* buf,int size)
{
    int n;
    int i = 0;
    char c = '0';
    while((i < size - 1) && (c != '\n'))
    {
        n = recv(cfd,&c,1,0);
        if(n > 0){
            if(c == '\r'){
                n = recv(cfd,&c,1,MSG_PEEK);
                if((n > 0) && (c=='\n'))
                    recv(cfd,&c,1,0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';
    return i;
}


const char *get_file_type(const char *name)
{
    char* dot;

    // 自右向左查找‘.’字符, 如不存在返回NULL
    dot = strrchr(name, '.');   
    if (dot == NULL)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp( dot, ".wav" ) == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}




//删除某个节点，关闭文件描述符
void disconnect(int cfd,int epfd)
{
    epoll_ctl(epfd,EPOLL_CTL_DEL,cfd,NULL);
    Close(cfd);
}




//发送协议--状态行、首部行、空格
void send_respond_head(int cfd,int no,const char*disp,const char* type,long len)
{
    char buf[1024] = {0};
    sprintf(buf, "HTTP/1.1 %d %s\r\n", no,disp);
    sprintf(buf+strlen(buf), "Content-Type:%s\r\n", type);
    sprintf(buf+strlen(buf), "Content-Length:%ld\r\n", len);
    sprintf(buf+strlen(buf), "\r\n");
    send(cfd,buf,strlen(buf),0);                            
}




//发送请求的数据
void send_file(int cfd,const char* file)
{
    char buf[4096] = {0};
    int len;
    int fd = open(file,O_RDONLY);
    if(fd == -1){
        perror("open error");
        exit(1);
    }
    while((len = Read(fd,buf,sizeof(buf))) > 0)
        send(cfd,buf,len,0);
    Close(fd);
}



void send_dir(int cfd,const char* dir)
{
    int i,ret;
    char buf[40960] = {0};
    char enstr[1024] = {0};
    char path[1024] = {0};

    sprintf(buf,"<html><head><title>目录名:%s</title></head>",dir);
    sprintf(buf+strlen(buf),"<body><h1>当前目录:%s</h1><table>",dir);

    struct dirent **ptr;//存放目录下所有文件的字符指针数组
    int num = scandir(dir,&ptr,NULL,alphasort);

    //遍历目录，把文件名挨个拿出来
    for(i = 0;i < num;i++)
    {
        char *name = ptr[i]->d_name;

        //拼接完整路径,获得path的属性
        sprintf(path,"%s/%s",dir,name);
        printf("path: %s\n",path);
        struct stat st;
        stat(path,&st);

        //编码生成类似于%4a,3d....
        encode_str(enstr,sizeof(enstr),name);

        //文件or目录
        if(S_ISREG(st.st_mode))
            sprintf(buf+strlen(buf), "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",enstr,name,st.st_size);
        if(S_ISDIR(st.st_mode)) 
            sprintf(buf+strlen(buf), "<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>",enstr, name, (long)st.st_size);
        
        ret = send(cfd,buf,strlen(buf),0);

         if (ret == -1) {
            if (errno == EAGAIN) {
                perror("send error:");
                continue;
            } else if (errno == EINTR) {
                perror("send error:");
                continue;
            } else {
                perror("send error:");
                exit(1);
            }
        }
        memset(buf, 0, sizeof(buf));
        // 字符串拼接
    }
    sprintf(buf+strlen(buf), "</table></body></html>");
    send(cfd, buf, strlen(buf), 0);
    printf("dir message send OK!!!!\n");
}



//发送404的html
void send_error(int cfd, int status, const char *title, const char *text)
{
    char buf[4096] = {0};
    sprintf(buf, "<html><head><title>%d %s</title></head>\n", status, title);
    sprintf(buf+strlen(buf), "<body bgcolor=\"#cc99cc\"><h2 align=\"center\">%d %s</h4>\n", status, title);
    sprintf(buf+strlen(buf), "%s\n", text);
    sprintf(buf+strlen(buf), "<hr>\n</body>\n</html>\n");
    send(cfd, buf, strlen(buf), 0);
}



//处理http请求
void http_request(const char* line,int cfd)
{
    //解析请求行
    char method[12], path[1024], protocol[12];
    sscanf(line, "%[^ ] %[^ ] %[^ ]", method, path, protocol);     
    printf("method = %s, path = %s, protocol = %s\n", method, path, protocol);

    // 转码 将不能识别的中文乱码 -> 中文
    // 解码 %23 %34 %5f
    decode_str(path, path);
    const char* file = path + 1;    //前边有个/，所以地址偏移1，取出文件名
    //如果没有指定路径，默认去当前目录
    if(strcmp(path,"/")==0){
        file = "./";
    }
    //判断文件目录是否存在
    struct stat sbuf;
    int ret = stat(file,&sbuf);
    if(ret != 0){
        send_respond_head(cfd,404,"File Not Find","Content-Type:text/html",-1);
        send_error(cfd,404,"Not Find","No Such File Or Direntry--haha--liu xiangle");
        return ;
    }
    //文件
    if(S_ISREG(sbuf.st_mode)){
        send_respond_head(cfd,200,"ok",get_file_type(file),sbuf.st_size);
        send_file(cfd,file);
    }
    //目录
    if(S_ISDIR(sbuf.st_mode)){
        send_respond_head(cfd,200,"ok",get_file_type(".html"),-1);
        send_dir(cfd,file);
    }
}




//epoll对就绪事件的分类处理
int do_epoll(int listen_fd,int epfd,int num,struct epoll_event evts[])//数组名传参时退化为指针
{
    struct epoll_event evt;
    for(int i = 0;i < num;i++)
    {
        if(evts[i].data .fd == listen_fd)
        {
            struct sockaddr_in client_addr;
            socklen_t len = sizeof(client_addr);
            int cfd = Accept(listen_fd,(struct sockaddr*)&client_addr,&len);
            printf("收到来自%s  [%d]的连接\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
            int flag = fcntl(cfd,F_GETFL,0);//非阻塞
            flag |= O_NONBLOCK;
            fcntl(cfd,F_SETFL,flag);
            evt.events = EPOLLIN | EPOLLET;//ET模式
            evt.data.fd = cfd;
            epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&evt);//上树
        }


        else
        {
            char line[1024] = {0};
            int len = get_line(evts[i].data.fd,line,sizeof(line));//返回一行的字符数，不带\r\n
            if(len == 0){
                printf("client close...\n");
                disconnect(evts[i].data.fd,epfd);
            }
            else{
                printf("%d---%s",len,line);
                while(len){
                    char buf[1024] = {0};
                    len = get_line(evts[i].data.fd,buf,sizeof(buf));
                    printf("%d---%s",len,buf);
                }
                if(strncasecmp(line,"GET",3) == 0){
                    http_request(line,evts[i].data.fd);
                    disconnect(evts[i].data.fd,epfd);
                }
            }

        }
    }
    return 0;
}



//epoll模型
void epoll_run(int port)
{
    int listen_fd = startup(port);
    struct epoll_event evt,evts[MAXSIZE];
    evt.events = EPOLLIN;
    evt.data.fd = listen_fd;
    int epfd = epoll_create(MAXSIZE);
    epoll_ctl(epfd,EPOLL_CTL_ADD,listen_fd,&evt);
    int timeout = 3000;
    while(1)
    {
        int num = epoll_wait(epfd,evts,MAXSIZE,timeout);
        switch (num)
        {
        case 0:
            printf("timeout\n");
            break;
        case -1:
            printf("epoll error\n");
            return;
        default:
            do_epoll(listen_fd,epfd,num,evts);
            break;
        }
    }
}



int main(int argc,char* argv[])
{
    if(argc < 3){
        printf("input like this ./a.out port path\n");
        return 0;
    }
    int port = atoi(argv[1]);//端口
    int ret = chdir(argv[2]);//改变工作目录，方便后续操作
    if(ret == -1){
        perror("chdir error");
        return 0;
    }

    epoll_run(port);//启动epool模型
    return 0;

}











































/*原型
  int epoll_create(int size);
  int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
  int epoll_wait(int epfd, struct epoll_event *events,int maxevents, int timeout);

  typedef union epoll_data {
  void        *ptr;
  int          fd;
  uint32_t     u32;
  uint64_t     u64;
  } epoll_data_t;

  struct epoll_event {
  uint32_t     events;      // Epoll events 
  epoll_data_t data;        // User data variable 
  };

*/
