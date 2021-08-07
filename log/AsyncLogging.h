/*
 * */
#ifndef _ANSYCLOGHING_H_
#define _ANSYCLOGHING_H_

#include"CountDownLatch.h"
#include"MutexLock.h"
#include"Thread.h"
#include"LogStream.h"
#include"noncopyable.h"
#include <functional>
#include <string>
#include <vector>
#include <memory>

class AsyncLogging:noncopyable
{
public:
    AsyncLogging(const std::string basename,int flushInterval=2);
	~AsyncLogging()
	{
		if(running_)
			stop();
	}
	void append(const char* logline,int len);//前端线程最下层的接口，与缓冲系统交互

	//开始启动日志系统
	void start()
	{
		running_=true;
		thread_.start(); // thread_.start返回说明后端线程创建
		latch_.wait();   // 前端线程先阻塞,等待后端线程唤醒，才能返回。目的是确保后端线程核心函数ThreadFunc传进去了，并启动成功。
	}
	void stop()
	{
		running_=false;
		cond_.notify();
		thread_.join();
	}



private:
    void threadFunc();//后端线程的最靠上接口。


private:
    using Buffer = FixedBuffer<kLargeBuffer>;
    using BufferVector = std::vector<std::shared_ptr<Buffer>>;
	using BufferPtr = std::shared_ptr<Buffer>;
    const int flushInterval_;//刷新间隔时间，缓冲区没写满，仍将缓冲区的数据写到文件中
	bool running_;// 是否正在运行
	std::string basename_;// 日志文件名字

	Thread thread_;// 执行该异步日志记录器的线程
	MutexLock mutex_;
	Condition cond_;
	CountDownLatch latch_;//倒数计数，用于指示什么时候日志记录器才能开始正常工作,用于等待线程启动


	BufferPtr currentBuffer_;//现有的缓冲区，智能指针4M
	BufferPtr nextBuffer_;//预备缓冲区，智能指针4M
	BufferVector buffers_;//缓冲区队列，待写入文件

};
#endif
