/* C++11的锁就是这么封装的，RAII手法 临界区构造加锁，超出作用域析构解锁*/

#ifndef _MUTEXLOCK_H_
#define _MUTEXLOCK_H_
#include"noncopyable.h"
#include<pthread.h>
#include<cstdio>

class MutexLock:noncopyable
{
public:
	MutexLock() {pthread_mutex_init(&mutex,NULL);}//构造函数动态初始化一把锁
	~MutexLock(){pthread_mutex_lock(&mutex);pthread_mutex_destroy(&mutex);}//析构函数销毁锁
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
	explicit MutexLockGuard(MutexLock& _mutex):mutex(_mutex) {mutex.lock();}//构造函数加锁
	~MutexLockGuard() {mutex.unlock();}//析构函数解锁
private:
	MutexLock& mutex;
};

#endif
