#ifndef _THREADPOLL_H_
#define _THREADPOLL_H_
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

/* IO线程池 && 业务线程池暂时都写在一起
 * 他们之间的通信，依赖条件变量&互斥锁进行
 */
namespace swings
{
    using IO_JobFunction = std::function<void()>;   //暂时当函数指针来用
    using Curd_JobFunction = std::function<void()>;

    class ThreadPool
    {
    public:
        ThreadPool(int numWorks);
        ~ThreadPool();
        void IO_pushJob(const IO_JobFunction& job);
        void Curd_pushJob(const Curd_JobFunction& job);

    private:
        std::vector<std::thread> threads_;          //存线程对象的数组
        std::queue<IO_JobFunction> IO_queues_;      //任务队列
        std::queue<Curd_JobFunction> Curd_queues_;  // 业务事件队列
        std::mutex IO_lock_;                        //IO互斥锁
        std::mutex Curd_lock_;                      //Curd互斥锁
        std::condition_variable IO_conn_;           //IO线程条件变量
        std::condition_variable Curd_conn_;         //Curd线程的条件变量
        bool stop_;                                 //线程池开关
    };
}


#endif
