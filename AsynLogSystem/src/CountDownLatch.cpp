#include"../include/CountDownLatch.h"
CountDownLatch::CountDownLatch(int count)
	:mutex_(), condition_(mutex_), count_(count) {}//初始化条件变量，以及条件变量里的锁

void CountDownLatch::wait()
{
    MutexLockGuard lock(mutex_);
	while(count_>0)
		condition_.wait();
}
void CountDownLatch::countDown()
{
	MutexLockGuard lock(mutex_);
	--count_;
	if(count_==0)
		condition_.notifyAll();
}
