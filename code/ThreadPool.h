#ifndef _THREADPOLL_H_
#define _THREADPOLL_H_

#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>


namespace swings
{
    using JobFunction = std::function<void()>;   //暂时当函数指针来用

    class ThreadPool
    {
    public:
        ThreadPool(int numWorks);
        ~ThreadPool();
        void pushJob(const JobFunction& job);

    private:
        std::vector<std::thread> threads_;       //存线程对象的数组
        std::queue<JobFunction> queues_;         //任务队列
        std::mutex lock_;                        //互斥锁
        std::condition_variable conn_;           //条件变量
        bool stop_;                              //线程池开关


    };
}


#endif
