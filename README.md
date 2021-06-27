# WebServer

## 简介

本项目是基于C++11的一个Web服务器,可处理静态请求，支持长连接，实现了异步日志，记录服务器的运行状态
测试连接：http://liuxiangle.pub

| PartI | Part II | PartIII | PartIV | partV | partVI |    
| :-------: | :-------: | :-----: | ----------: | -------- | -------- | 
|  [并发模型](https://github.com/liu-9969/WebServer/blob/master/%E5%B9%B6%E5%8F%91%E6%A8%A1%E5%9E%8B.md)  | [核心类](https://github.com/liu-9969/WebServer/blob/master/%E6%A0%B8%E5%BF%83%E7%B1%BB.md)| [整体架构](https://github.com/liu-9969/WebServer/commit/c9b80aec44a3c260676b0837acbf28b429fb9d17) |[困难]https://github.com/liu-9969/WebServer/blob/master/%E9%81%87%E5%88%B0%E7%9A%84%E5%9B%B0%E9%9A%BE.md|[并发测试](https://github.com/liu-9969/WebServer/blob/master/%E5%B9%B6%E5%8F%91%E6%B5%8B%E8%AF%95.md)      | [项目目的](https://github.com/liu-9969/WebServer/blob/master/%E9%A1%B9%E7%9B%AE%E7%9B%AE%E7%9A%84.md)         |   


## 代码统计
## 开发工具
* Ubuntu18.04
* vim
* g++8.4.0
* make
* gdb
* webbench
## 待优化
* 多线程异步日志
* 为不提供拷贝构造和赋值运算符的类添加noncopyable基类
## 技术纲要
* 并发模型：单Reactor多线程模型（同步）
* 网络模式：多路Epoll(LT)IO + EPOLLONESHOT + 非阻塞IO + readv、writev
* 线程池：C++11的线程库
* 缓冲区：复刻了muduo的设计思想，双缓冲（vector+栈上64K）
* 时钟：C++11的时间库
## 数据结构：
* **vector**: buff缓冲区的核心，能够实现自动增长；存储线程的数组等等
* **map**:                     存储http请求报文的头部，key存储字段，value存储后边的内容。这样在判断一个连接是否是长连接时很快，
* **priority_queue**:  
          按照所剩时间的优先级，存储定时器。这样队列内部帮我们排好序了，我们用的时候就比较容易的知道离下个超时还有多久，
                       拿这个时间作为epoll的参数很合适
* **queue**：          线程池中存储任务回调函数。
* **string**：解析报文用的较多 

