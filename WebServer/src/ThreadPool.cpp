#include "../include/ThreadPool.h"
#include "../../AsynLogSystem/include/Logging.h"
#include<sys/prctl.h>
using namespace swings;



ThreadPool::ThreadPool(int numWorks):stop_(false)
{
    numWorks = numWorks <= 0 ? 1 : numWorks;
    // IO线程
    for(int i = 0;i < numWorks;i++ )
    {
        threads_.emplace_back([this]{
            LOG<<CurrentThread::tid()<<"  IO_thread creat successfuly";
            prctl(PR_SET_NAME, "IO");//赋予线程的名子
            // IO线程是一个while(1)等待任务，处理任务的cpu操作
            while(1)
            {
                IO_JobFunction func;
                std::unique_lock<std::mutex> lock(IO_lock_);
                while(IO_queues_.empty() && !stop_)
                    IO_conn_.wait(lock);
                if(IO_queues_.empty() && stop_){
                    LOG<<CurrentThread::tid()<<"  thread return"; 
                    return ;
                }
                func = IO_queues_.front();
                IO_queues_.pop();
                if(func)
                    func();
            }
        }); 
    }

    // 业务线程
    for(int i = 0;i < 1;i++ )
    {
        threads_.emplace_back([this]{
            LOG<<CurrentThread::tid()<<"  CURD_thread creat successfuly"; 
            prctl(PR_SET_NAME, "Curd");
            // 
            while(1)
            {
                Curd_JobFunction func;
                std::unique_lock<std::mutex> lock(Curd_lock_);
                while(Curd_queues_.empty() && !stop_)
                    Curd_conn_.wait(lock);
                if(Curd_queues_.empty() && stop_){
                    LOG<<CurrentThread::tid()<<"  thread return"; 
                    return ;
                }
                func = Curd_queues_.front();
                Curd_queues_.pop();

                if(func){
                    func();
                }
            }
        }); 
    }


}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(Curd_lock_);
        stop_ = true;
    }
    Curd_conn_.notify_all();//唤醒等待该条件的所有线程，若没有什么也不做
    for(auto &thread:threads_){
        thread.join();//让主控线程等待（阻塞）子线程退出
    }
    printf("[Threadpool::~Threadpool] Threadpool is remove\n");
}

void ThreadPool::IO_pushJob(const IO_JobFunction& job)
{
    {
        std::unique_lock<std::mutex> lock(IO_lock_);
        IO_queues_.push(job);
    }
    IO_conn_.notify_one();
}

void ThreadPool::Curd_pushJob(const Curd_JobFunction& job)
{
    {
        std::unique_lock<std::mutex> lock(Curd_lock_); 
        Curd_queues_.push(job);
    }
    Curd_conn_.notify_one();
}












