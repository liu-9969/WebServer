#include "ThreadPool.h"

using namespace swings;



ThreadPool::ThreadPool(int numWorks):stop_(false)
{
    numWorks = numWorks <= 0 ? 1 : numWorks;
    for(int i = 0;i < numWorks;i++ )
    {
        threads_.emplace_back([this]{
            while(1)
            {
                JobFunction func;
                std::unique_lock<std::mutex> lock(lock_);
                while(queues_.empty() && !stop_){
                    conn_.wait(lock);
                }
                if(queues_.empty() && stop_){
                    //printf("[Threadpool::Threadpool]thid:%lu return",pthread_self());
                    return ;
                }

                func = queues_.front();
                queues_.pop();

                if(func){
                    //printf("[Threadpool::Threadpool]thid:%lu get a job\n",pthread_self());
                    func();
                    //printf("[Threadpool::Threadpool]thid:%lu job finish\n",pthread_self());
                }
            }
        }); 
    }
}




ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(lock_);
        stop_ = true;
    }
    conn_.notify_all();//唤醒等待该条件的所有线程，若没有什么也不做
    for(auto &thread:threads_){
        thread.join();//让主控线程等待（阻塞）子线程退出
    }
    printf("[Threadpool::~Threadpool] Threadpool is remove\n");
}



void ThreadPool::pushJob(const JobFunction& job)
{
    {
        std::unique_lock<std::mutex> lock(lock_);
        queues_.push(job);
    }
    //printf("[Threadpool::pushJob]push new job\n");
    conn_.notify_one();
}













