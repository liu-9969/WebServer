/*************************************************
 *头文件：重新封装一些系统调用
 *创建时间：2021-03-14
 *修改时间：2021-03-14
 *
 *************************************************/

#ifndef _WRAP_H_
#define _WRAP_H_
#include <unistd.h>

void err_exit(const char* s);
int Socket(int domain, int type, int protocol);
int Bind(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
int Listen(int sockfd, int backlog);
int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int Connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
int Close(int fd);
ssize_t Read(int fildes, void *buf, size_t nbyte);
ssize_t Write(int fildes, const void *buf, size_t nbyte);
ssize_t Readn(int fd, void *vptr, size_t n);
ssize_t Writen(int fd, const void *vptr, size_t n);
ssize_t my_read(int fd, char *ptr);
ssize_t Readline(int fd, void *vptr, size_t maxlen);

#endif



