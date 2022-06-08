#include <pthread.h>
#include <assert.h>
#include "../include/Thread.h"
#include "../include/CurrentThread.h"
#include<sys/prctl.h>
#include<stdint.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <syscall.h>

using namespace CurrentThread;

pid_t gettid()//[]//
{
	return static_cast<pid_t>(::syscall(SYS_gettid));  //tid这个值只能通过linux的系统调用取得，syscall(SYS_gettid)
}
namespace CurrentThread
{
	__thread int t_cachedTid = 0;//缓存的tid
	__thread char t_tidString[32];//tid的字符串表示形式
	__thread int t_tidStringLength = 6;//线程id的长度
	__thread const char* t_threadName = "default";//线程的名称默认
}
void CurrentThread::cacheTid()//缓存tid,整数打印到字符串，前三个变量就初始化了呗
{
	if (t_cachedTid == 0) {
		t_cachedTid = gettid();
		t_tidStringLength = snprintf(t_tidString, sizeof(t_tidString), "%5d", t_cachedTid);
	}
}

//中间件
struct ThreadData
{

    typedef Thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;
    string name_;
    pid_t* tid_;
    CountDownLatch* latch_;
    
    ThreadData(const ThreadFunc& func,const string& name,pid_t* tid,CountDownLatch* latch)
        :func_(func),
         name_(name),
         tid_(tid),
         latch_(latch){}
    
    void runInThread()//线程运行
    {
      *tid_ = CurrentThread::tid();//传出参数套取出tid
      tid_ = NULL;
      latch_->countDown();
      latch_ = NULL;

      CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str();
      prctl(PR_SET_NAME, CurrentThread::t_threadName);//赋予线程的名子

      func_();//希望它执行的函数
      CurrentThread::t_threadName = "finished";//赋予运行完的线程名
    }
};


//线程回调函数，对runINThread()的封装，因为pthread_create只接收静态函数
void* startThread(void* obj)
{
	ThreadData* data = static_cast<ThreadData*>(obj);
	data->runInThread();
	delete data;
	return NULL;
}




//Thread类成员函数的定义
Thread::Thread(const ThreadFunc& func,const string& n)
    :started_(false),
     joined_(false),
     pthreadId_(0),
     tid_(0),
     func_(func),
     name_(n),
     latch_(1)
{
    setDefaultName();
}

Thread::~Thread() 
{
  if (started_ && !joined_) pthread_detach(pthreadId_);
}

void Thread::setDefaultName()
{
  if (name_.empty()) 
  {
    char buf[32];
    snprintf(buf, sizeof buf, "Thread");
    name_ = buf;
  }
}

int Thread::join() 
{
  assert(!started_);
  assert(!joined_);
  joined_ = true;
  return pthread_join(pthreadId_, NULL);
}

void Thread::start()
{
    assert(!started_);
    started_ = true;
    ThreadData* data = new ThreadData(func_,name_,&tid_,&latch_); // 【前端线程】预先创建缓存前端线程的结构，传给后端线程
    if(pthread_create(&pthreadId_,NULL,&startThread,data)) {      // 创建线程失败
        started_ = false;
        delete data;
    }else{
        latch_.wait(); // 前端程挂起先等等后端线程唤醒，这段时间就是日志线程在往缓冲区写线程id，直到log线程。。。
        assert(tid_ > 0);
    }
}







































