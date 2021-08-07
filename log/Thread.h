/************************************* 线程的封装 ****************************************
 *
 */
#ifndef _THREAD_H_
#define _THREAD_H_

#include <functional>
#include "noncopyable.h"
#include "CountDownLatch.h"

using std::string;
class Thread:noncopyable
{
    public:
        using ThreadFunc = std::function<void()>;

        explicit Thread(const ThreadFunc&,const string& name = string()); // 只做初始化
        ~Thread();             // 程序不崩，它不会被调用

        void start();          // 【这个类最核心的函数】，负责创建线程
        int join();            // 程序不崩，它不会被调用

    private:
        void setDefaultName();


    private:
        bool started_;         // 标识子(日志)线程是否已经被创建，就刚初始化那会假，跑起来就一直为真了。
        bool joined_;          // 标识子(日志)线程是否已经被回收，只要程序不崩掉，join它一辈子都是假的。
        
        pthread_t pthreadId_;  // 同一进程里唯一
        pid_t tid_;            // 真正的线程id,linux下线程就是轻量级进程麻，个人理解pid_t就是描述线程id的，是进程沾了它的光。

        ThreadFunc func_;      // 线程最核心的函数，就是那个threadFunc()
        
        string name_;          // 线程名字，这个纯属子虚乌有
        CountDownLatch latch_; // 就直接说它是条件变量得了

        //有没有发现，线程没封装锁、条件变量？

};

#endif
