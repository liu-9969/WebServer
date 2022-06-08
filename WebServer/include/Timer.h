//@liu xiangle
//@2021-5-14

#ifndef _TIMER_H_
#define _TIMER_H_

#include <chrono>
#include <functional>
//#include <assert.h>
#include <cassert>
#include <queue>
#include <vector>
#include <mutex>

namespace swings
{
    using TimeoutCallBack = std::function<void()>;      //回调，函数指针
    using MS = std::chrono::milliseconds;               //单位，毫秒
    using Clock = std::chrono::high_resolution_clock;   //时钟
    using Timestamp = Clock::time_point;                //时间点,距时间戳的时间段
    class HttpRequest;


    class Timer
    {
    public:
        Timer(const Timestamp& when,const TimeoutCallBack cb)
            :expireTime_(when),
             callBack_(cb),
             delete_(false){}
        ~Timer(){}

        void del() {delete_ = true;}
        bool isDelete() {return delete_;}
        Timestamp getExpireTime() const {return expireTime_;}
        void runCallBack() {callBack_();}

    private:
        Timestamp expireTime_;       //超时时间点
        TimeoutCallBack callBack_;   //回调对象，函数指针
        bool delete_;                //开关
    };


    struct cmp
    {
        bool operator()(Timer* a,Timer* b)
        {
            assert(a != nullptr && b != nullptr);
            return (a->getExpireTime()) > (b->getExpireTime());
        }
    };


    class TimerManager
    {
    public:
        TimerManager()
            :now_(Clock::now()){}
        ~TimerManager(){}

        void updataTimer() {now_ = Clock::now();}
        void addTimer( HttpRequest* request,const int& timeout,const TimeoutCallBack& cb );
        void delTimer( HttpRequest* reques );
        void handleTimer();
        int getNextExpireTime();  

    private:
        //stl中的优先队列，不按照先进先出原则，优先级高的元素先出队
        using TimeQueue = std::priority_queue<Timer*,std::vector<Timer*>,cmp>;
        TimeQueue timerQueue_;          //计时器队列
        Timestamp now_;               //当前时间
        std::mutex lock_;             //互斥锁，各线程进临界区必须先拿锁
    };
}


#endif

