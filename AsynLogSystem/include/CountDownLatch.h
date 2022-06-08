 /*
  * 本质：其实它就是对条件变量包了一下，本质是去解决的（刚开始写时）前后端线程同步的问题。
  * 作用：确保Thread中传进去的threadFunc()真的启动了以后,外层的start你能才返回。 
  * 设计：在后端线程函数刚开始要执行的时候，它通知前端线程写可以返回了。
  */

#ifndef _COUNTDOWNLATCH_H_
#define _COUNTDOWNLATCH_H_
#include"Condition.h"
#include"MutexLock.h"
#include"noncopyable.h"

#include"Condition.h"
#include"MutexLock.h"
#include"noncopyable.h"

class CountDownLatch:noncopyable
{
public:
	explicit CountDownLatch(int count);
	void wait();              //让线程先阻塞的
	void countDown();         //-1，到0唤醒所有线程
private:
	mutable MutexLock mutex_; //锁
	Condition condition_;     //条件变量
	int count_;               //倒数，用1多
};

#endif

