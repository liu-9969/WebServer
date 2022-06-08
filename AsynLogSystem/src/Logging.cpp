#include"../include/Logging.h"
#include"../include/CurrentThread.h"
#include"../include/Thread.h"
#include"../include/AsyncLogging.h"
#include<assert.h>
#include<iostream>
#include<time.h>
#include<sys/time.h>

static pthread_once_t once_control_=PTHREAD_ONCE_INIT;
static AsyncLogging* AsyncLogger_; // 【静态全局，唯一】

std::string Logger::logFileName_="log.txt"; //【静态成员变量，LOG析构他也存在】




// once_init只会被第一个线程调用一次，后边的线程不会调用它，
// 保证AsyncLogger只被创建一次，被初始化一次。【优化考虑单例模式】
void once_init()
{
	AsyncLogger_=new AsyncLogging(Logger::getLogFileName()); // 日志文件名往下传递
	AsyncLogger_->start(); //【启动后端系统，它返回意味着后端线程创建成功，并且ThreadFunc函数载入成功并在执行】
}



// LOG析构时调用，并最后追加了行号、源文件名
void output(const char* msg,int len)
{
	pthread_once(&once_control_,once_init);
	AsyncLogger_->append(msg,len);
}



// 【@@】构造Impl时，往缓冲追加时间信息。
Logger::Impl::Impl(const char*fileName,int line)
	:stream_(),
	 line_(line),
	 basename_(fileName)
{
	formatTime();
}


//格式化当前时间
void Logger::Impl::formatTime()
{
	struct timeval tv;
	time_t time;
	char str_t[26]={0};
	gettimeofday(&tv,NULL);// 返回当前距离1970年的秒数和微妙数，后面的tz是时区，一般不用。
	time=tv.tv_sec; //取得从1970年1月1日至今的秒数。
	struct tm* p_time=localtime(&time);//将time_t表示的时间转换为经过时区转换的UTC时间
	strftime(str_t,26,"%Y-%m-%d %H:%M:%S  ",p_time);
	stream_<<str_t;
}

//构造时，会自动载入行号、源文件名。
Logger::Logger(const char *fileName,int line)
	:impl_(fileName,line)
    //pid_(CurrentThread::tid())
{
    //impl_.stream_<<pid_;
}

// 析构时，在日志末尾追加行号，源文件名
Logger::~Logger()
{
	impl_.stream_<<" -- "<<impl_.basename_<<':'<<impl_.line_<<'\n';//\n用来日志消息换行。
	const LogStream::Buffer& buf(stream().buffer());//定义引用buf,用本类中的buf去初始化他。
	output(buf.data(),buf.length());//由append()去最复制到缓冲区系统。
}



































/********************************************************************************************************************

   muduo::Logger(__FILE__, __LINE__).stream() << "This is a log."将这句话放在日志打印的地方意味着什么呢？

    1.创建一个 Logger(FILE, LINE) 的匿名对象；
    2.调用这个匿名对象的 stream() 成员函数；
    3.调用重载后的 « 操作符，输入日志信息；
    4.析构该匿名对象，析构函数内会调用真正的output日志信息函数。
    注意，匿名对象的析构发生在该条语句结束后，实际上在实现上，前3步的目的是整合拼接一条完整的日志信息，
    实际的日志打印动作就发生在第4步析构上。这种手法初看比较新颖，但如果把日志对象看做一种资源的话，可以理解为RAII手法的变种。


   1.gettimeofday()会把目前的时间用tv结构体返回，当地时区的信息则放到tz所指的结构中
    原型：int gettimeofday(struct timeval*tv, struct timezone *tz);

    2.strftime（）函数将时间格式化为我们想要的格式
    原型：size_t strftime(char *strDest, size_t maxsize, const char *format, conststruct tm *timeptr);
    %Y:不带世纪的十进制年份（值从0到99)
    %m 十进制表示的月份
    %d十进制表示的每月的第几天
    %H 24小时制的小时
    %M 十时制表示的分钟数
    %S 十进制的秒数

    struct timeval
    {
        time_t tv_sec;
        suseconds_t tv_usec;
    };
     struct tm
     {
        int tm_sec;    Seconds. [0-60] (1 leap second)
        int tm_min;    Minutes. [0-59]
        int tm_hour;   Hours.   [0-23]
        int tm_mday;   Day.     [1-31]
        int tm_mon;    Month.   [0-11]
        int tm_year;   Year - 1900.
        int tm_wday;   Day of week. [0-6]
        int tm_yday;   Days in year.[0-365]
        int tm_isdst;  DST.     [-1/0/1]
    };

***************************************************************************************************************/


