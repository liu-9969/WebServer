# 高性能WebServer

## 简介

1. 本项目是基于C++11的一个高性能Web服务器
   * 业务：B/S模型，可处理http的get post请求，支持长连接,提供了一些选课后端接口
   * 日志：实现了异步日志，记录服务器的运行状态
   * 框架：单Reactor多线程模型；部分模块学习了muduo网络库做法
   * 性能：QPS 10000+
   * 目的：学习C++服务端编程，对高并发有一个入门思考

2. 测试连接：http://liuxiangle.pub（域名已失效）



## 代码统计

![image-20220421162410450](C:\Users\liu-cc\AppData\Roaming\Typora\typora-user-images\image-20220421162410450.png)

## 工具

* 操作系统：阿里云Centos 8.2
* 编辑工具：vim、vscode
* 编译工具：g++8.4.0、make
* 调试工具：GNU gdb8.1.1
* 压测工具：webbench1.5、ab



## 技术纲要
* 并发模型：  单Reactor多线程模型（同步）
* 网络模式：  多路IO epoll(ET+EPOLLONESHOT) + 非阻塞IO + 高级读写IO + 存储映射IO
* 线程池：    IO线程 + 业务线程
* 连接池:     单例模式
* 应用层缓冲： 基于双缓冲技术 vector+栈64K
* 定时器：    基于C++11的chrono时间库，小根堆管理定时器，方便获取最短时间给epoll_wait
* 异步日志：  基于多缓冲区技术，IO/LOG线程交替使用，实现异步---写日志不阻塞
* 线程安全：  智能指针、RAII机制、互斥锁、条件变量
* 后端接口：  Rest风格，JsonCpp库



## High Performance的入门及思考

* 多线程+多路IO非阻塞：优点在哪？性能瓶颈在哪？适合的应用场景是什么？

* 协程和异步的学习，在高并发场景下，相较线程，它的优势是什么？

* 单机的瓶颈，横向扩展，分布式系统的学习

* 一些值得阅读的开源项目：

* [c/c++值得学习的开源框架和库](https://github.com/0voice/developkit_set)

  * muduo:c++编写的linux多线程服务器网络库

  * nginx：高性能的HTTP和反向代理web服务器，同时也提供了IMAP/POP3/SMTP服务。

  * libevent/libev/libuv：c语言实现的网络库（他是同步的，假异步）

  * libgo/libco:c++11编写的协程库

## 架构图
![image](https://user-images.githubusercontent.com/68007721/137368619-9d50f265-27c0-4240-bf45-8f25ee84f1c8.png)
    