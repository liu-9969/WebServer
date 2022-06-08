/* C++11的锁就是这么封装的，RAII手法 临界区构造加锁，超出作用域析构解锁*/

#ifndef _MUTEXLOCK_H_
#define _MUTEXLOCK_H_
#include"noncopyable.h"
#include<pthread.h>
#include<cstdio>

class MutexLock:noncopyable
{
public:
	MutexLock() {pthread_mutex_init(&mutex,NULL);}
	~MutexLock(){pthread_mutex_lock(&mutex);pthread_mutex_destroy(&mutex);}
	void lock() {pthread_mutex_lock(&mutex);}
	void unlock(){pthread_mutex_unlock(&mutex);}
private:
	pthread_mutex_t mutex;
private:
	friend class Condition;
};

class MutexLockGuard:noncopyable
{
public:
	explicit MutexLockGuard(MutexLock& _mutex):mutex(_mutex) {mutex.lock();}
	~MutexLockGuard() {mutex.unlock();}
private:
	MutexLock& mutex;
};
// 注释：
// MutexLock是一个内部类，对外不使用，对外使用MutexLockGuard
// MutexLock 里面，构造函数初始化一把锁，析构函数销毁一把锁
// MutexLockGuard里面，构造函数加锁，析构函数解锁。
// 为什么要这么做？

#endif
