#pragma once
#include"LogStream.h"
#include<pthread.h>
#include<string.h>
#include<string>
#include<stdio.h>
#include "CurrentThread.h"


//Logging是对外接口，它定义了使用日志的方式，内含一个LogStream对象，并且在每次使用日志的时候在内容中加上时间、文件名、行号等内容。
class AsyncLogging;

class Logger
{
public:
	Logger(const char *fileName,int line);
	~Logger();
	LogStream& stream() { return impl_.stream_;}//拿到LogStream的这个对象
	static void setLogFileName(std:: string fileName) { logFileName_=fileName;}
	static std::string getLogFileName() { return logFileName_;}
private:
	//Impl技法，数据和对象分离 Impl对象存储的就是Logger所需要的所有数据
	class Impl
	{
	public:
		Impl(const char* fileName,int line);
		void formatTime();//格式化时间
		LogStream  stream_;//构造日志缓冲区，该缓冲区重载了各种<<，都是将数据格式到LogStream的内部成员缓冲区buffer里
		int line_;//行
		std:: string basename_;//基本名称
	};
	Impl impl_;//logger构造这个对象
	//pid_t pid_;//线程id
	static std::string logFileName_;
};

#define LOG Logger(__FILE__,__LINE__).stream()
//宏替时首先会构造一个匿名对象，
//该对象调用stram()方法，返回Logstream对象，该对象重载了<<,还持有一个缓冲区。
//_FILE_:C语言中，用来指示本语句所在的文件名
//_LINE_:C语言中，用来指示本语句所在的行
