# 高性能WebServer

## 简介

1. 本项目是基于C++11的一个高性能Web静态服务器
   * 业务：B/S模型，可处理http的get head请求，支持长连接
   * 日志：实现了异步日志，记录服务器的运行状态
   * 框架：将**muduo网络库--陈硕**的主要框架抽出来实现了一遍
   * 性能：QPS 10000+
   * 目的：学习C++服务器编程
2. 测试连接：http://liuxiangle.pub（域名已失效）

| PartI | Part II | PartIII | PartIV | partV | partVI |    
| :-------: | :-------: | :-----: | ----------: | -------- | -------- | 
|  [并发模型](https://github.com/liu-9969/WebServer/blob/master/%E5%B9%B6%E5%8F%91%E6%A8%A1%E5%9E%8B.md)  | [核心类](https://github.com/liu-9969/WebServer/blob/master/%E6%A0%B8%E5%BF%83%E7%B1%BB.md)| [整体架构](https://github.com/liu-9969/WebServer/commit/c9b80aec44a3c260676b0837acbf28b429fb9d17) |[困难](https://github.com/liu-9969/WebServer/blob/master/%E9%81%87%E5%88%B0%E7%9A%84%E5%9B%B0%E9%9A%BE.md)|[并发测试](https://github.com/liu-9969/WebServer/blob/master/%E5%B9%B6%E5%8F%91%E6%B5%8B%E8%AF%95.md)      | [项目目的](https://github.com/liu-9969/WebServer/blob/master/%E9%A1%B9%E7%9B%AE%E7%9B%AE%E7%9A%84.md)         |   

## 代码统计

![在这里插入图片描述](https://img-blog.csdnimg.cn/e6ec0b74b4b7479f9139b5726a2faf50.png?x-oss-process=image/watermark,type_ZHJvaWRzYW5zZmFsbGJhY2s,shadow_50,text_Q1NETiBA5bCPbGl15ZCR5YyX,size_20,color_FFFFFF,t_70,g_se,x_16)

## 工具

* 操作系统：Ubuntu 18.04
* 编辑工具：vim vscode
* 编译工具：g++8.4.0、make
* 调试工具：GNU gdb8.1.1
* 压测工具：webbench1.5



## 技术纲要
* 并发模型：单Reactor多线程模型（同步）
* 网络模式：多路IO epoll(ET+EPOLLONESHOT) + 非阻塞IO + 高级读写IO
* 线程池：基于C++11的Thread线程库，生产者消费者模型，固定数量线程
* 应用层缓冲：基于双缓冲技术 vector+栈64K
* 磁盘IO：mmap存储映射IO
* 定时器：基于C++11的chrono时间库，小根堆管理定时器，方便获取最短时间给epoll_wait
* 异步日志：基于多缓冲区技术，IO/LOG线程交替使用，实现异步---写日志不阻塞
* 线程安全：1.智能指针、RAII机制、对象池等去保证析构的正确性 2.同步采用互斥锁条件变量
* tcp连接：一条连接绑定一个http解析器，同生死，RAII手法的思想



## High Performance的入门和思考

* 多线程+多路IO非阻塞（同步）：优点在哪？性能瓶颈在哪？适合什么应用场景？

* 协程是什么，在高并发场景下，相较线程，它的优势是什么？

* 异步是什么，他为什么比同步效率高，适用于什么场景

* 单机有没有真正意义的高并发？分布式系统的学习

* 一些值得阅读的开源项目：

* [c/c++值得学习的开源框架和库](https://github.com/0voice/developkit_set)

  * muduo:c++编写的linux多线程服务器网络库

  * nginx：高性能的HTTP和反向代理web服务器，同时也提供了IMAP/POP3/SMTP服务。

  * libevent/libev/libuv：c语言实现的网络库（他是同步的，虽然很多人说他是异步）

  * libgo/libco:c++11编写的协程库

    



## 数据结构：
* **vector**: 1.buff缓冲区的核心，能够实现自动增长；2.存储线程对象的数组......
* **map**: 存储http的请求报头、网络文件格式和本地格式表。
* **priority_queue**: 按照所剩时间的长短，存储定时器。
* **queue**：线程池中存储任务回调函数。
* **string**：解析报文用的较多。

