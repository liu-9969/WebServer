/* 
    2021-8-1@@liu-yy

    1._thread的作用：他就是线程的局部变量，每个线程都有一份他的拷贝，生命周期伴随线程到终止，它可以很方便的解决编程的一些麻烦。
       比如不用它，那么我们怎么实现前端业务线程的id输出到日志？
       1.在线程函数不行吧？很多线程可能用同一个回调函数；
       2.全局数组保存tid? 那要不要枷锁呢，怎么存放才能让线程正确访问呢
       3.所以，ChenShuo yyds 牛

    2.个人对这个CurrentThread命名空间的理解：
       顾名思义，Log日志库在此命名空间下封装了缓存线程信息的接口，避免为获得tid而重复的系统调用
       使用：全局函数tid()，随处调用，没啥顾虑。

*/
#ifndef _CURRENTTHREAD_H_
#define _CURRENTTHREAD_H_
namespace CurrentThread
{
    
    extern __thread int t_cachedTid;
    extern __thread char t_tidString[32];
    extern __thread int t_tidStringLength;
    extern __thread const char* t_threadName;

    void cacheTid();

    inline const char* tidString() {return t_tidString;}
	inline int tidStringLength() {return t_tidStringLength;}
	inline const char*name() {return t_threadName;}

    inline int tid()
	{
		if(__builtin_expect(t_cachedTid==0,0))//这里表明，t_cacheTid==0的概率很小，不会去先取下一行指令cacheTid()。等价与if(t_cacheTid==0)很小的概率
			cacheTid();
		return t_cachedTid;
	}
}
#endif
