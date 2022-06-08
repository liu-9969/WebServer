//@liu xiangle
//@2021-5-14

#include "../include/Timer.h"
#include "../include/HttpRequest.h"
#include "../../AsynLogSystem/include/Logging.h"
#include <cassert>
#include <iostream>

using namespace swings;

void TimerManager::addTimer( HttpRequest *request, const int &timeout, const TimeoutCallBack &cb )
{
    std::unique_lock<std::mutex> lock(lock_);
    assert(request != nullptr);
    updataTimer();

    Timer* timer = new Timer(now_ + MS(timeout),cb);
    timerQueue_.push(timer);
    if(request->getTimer() != nullptr)
        delTimer(request);
    request->setTimer(timer);
}




// 这个函数不必上锁，没有线程安全问题
// 若上锁，反而会因为连续两次上锁造成死锁：handleExpireTimers -> runCallBack -> __closeConnection -> delTimer
// 惰性删除，通过delete开关标量限制访问
// 如果delete掉，会造成悬垂指针,// 防止request -> getTimer()访问到垂悬指针 
// 如果要删除这个对象，必须先到对头，然后出队后，delete这根指针
void TimerManager::delTimer( HttpRequest *request )
{
    assert(request != nullptr);
    Timer* timer = request->getTimer();
    if(timer == nullptr)
        return;
    timer->del();
    request->setTimer(nullptr);
}



void TimerManager::handleTimer()
{
    std::unique_lock<std::mutex> lock(lock_);
    updataTimer();
    while(!timerQueue_.empty())
    {
        Timer* timer = timerQueue_.top();
        assert(timer != nullptr);
        if(timer -> isDelete()){
            timerQueue_.pop();
            delete timer;
            continue;
        }
        if(std::chrono::duration_cast<MS>(timer -> getExpireTime() - now_).count() > 0)
            return;
        // 超时
        timer -> runCallBack();
        LOG<<CurrentThread::tid()<<"  超时断开连接";       
        timerQueue_.pop();
        delete timer;
    }
}




// getNextExpireTime用于获取最近的超时时间，用于设置epoll_wait超时时间
int TimerManager::getNextExpireTime()
{
    std::unique_lock<std::mutex> lock(lock_);
    updataTimer();
    int res = -1;
    while(!timerQueue_.empty())
    {
        Timer* timer = timerQueue_.top();
        if(timer -> isDelete())
        {
            timerQueue_.pop();
            delete timer;
            continue;
        }
        res = std::chrono::duration_cast<MS>(timer -> getExpireTime() - now_).count();
        res = (res < 0) ? 0 : res;
        break;
    }
    return res;
}






