/*
  条件变量本身不是锁！但它也可以造成线程阻塞。通常与互斥锁配合使用。
  它可以起到给多线程提供一个会合的场所，配合mutex使用可以减少一些不必要的竞争。
*/
#ifndef _CONDITION_H_
#define _CONDITION_H_

#include<pthread.h>
#include<errno.h>
#include<cstdint>
#include<time.h>
#include"noncopyable.h"
#include"MutexLock.h"

class Condition:noncopyable
{
public:
	explicit Condition(MutexLock& _mutex):mutex(_mutex)   //构造函数初始化条件变量:mutex(_mutex)  
    {pthread_cond_init(&cond,NULL);}
	~Condition()  {pthread_cond_destroy(&cond);}          //析构函数销毁条件变量

	void wait() {pthread_cond_wait(&cond,&mutex.mutex);}  //阻塞等待条件变量满足+释放锁，满足后自动加锁
	void notify() {pthread_cond_signal(&cond);}           //唤醒至少一个阻塞在该条件变量的线程
	void notifyAll() {pthread_cond_broadcast(&cond);}     //唤醒全部阻塞在该条件变量的线程
	bool waitForSeconds(int seconds)                      //限时等待一个条件变量
	{
		struct timespec abstime;//存放时间点的结构体
		clock_gettime(CLOCK_REALTIME,&abstime);//获取当前系统的秒数与纳秒数。
		abstime.tv_sec+=static_cast<time_t>(seconds);//秒+秒
		return ETIMEDOUT==pthread_cond_timedwait(&cond,&mutex.mutex,&abstime); //#define ETIMEDOUT  110   /* Connection timed out */
	}
private:
	MutexLock& mutex;
	pthread_cond_t cond;
};

#endif
