/*
 * */
#ifndef _LOGSTREAM_H_
#define _LOGSTREAM_H_

#include <string>
#include <string.h>
#include "noncopyable.h"

class AsyncLogging;
const int kSmallBuffer=4000;
const int kLargeBuffer=4000*1000;

template<int SIZE>
class FixedBuffer:noncopyable 
{
    public:
	FixedBuffer() :cur_(data_) { }//初始化cur_为缓冲区开头
	~FixedBuffer(){ }
	void append(const char*buf,size_t len)
    {
		if(avail()>static_cast<int>(len))
        {
			memcpy(cur_,buf,len);//从buf指向的缓冲区复制len个字节到cur_指向的缓冲区
			cur_+=len;
		}
	}

	const char* data()const { return data_;}                     //向外暴露缓冲区首地址
	int length() const { return static_cast<int>(cur_ - data_); }//已用长度
	char* current() { return cur_;}                              //向外暴露cur位置
	int avail()const { return static_cast<int>(end() - cur_); }  //可用长度
	void add(size_t len) { cur_+=len;}
	void reset() { cur_=data_;}
	void bzero() { memset(data_, 0, sizeof(data_)); } 


private:
	const char* end() const { return data_+sizeof(data_);}//返回数组最后一个元素的下一个元素的地址，不算越界
	
private:
    char data_[SIZE];  //这个类的核心
	char* cur_;        //下一个要写的位置
};


//************************************************************************************************************

class LogStream:noncopyable
{
public:
	typedef FixedBuffer<kSmallBuffer> Buffer;
	LogStream& operator<<(bool v)
	{
		buffer_.append(v?"1":"0",1);
		return *this;
	}

	LogStream& operator<<(short);
	LogStream& operator<<(unsigned short);
	LogStream& operator<<(int);
	LogStream& operator<<(unsigned int);
	LogStream& operator<<(long);
	LogStream& operator<<(unsigned long);
	LogStream& operator<<(long long);
	LogStream& operator<<(unsigned long long);
	LogStream& operator<<(double);
	LogStream& operator<<(long double);
	//LogStream& operator<<(const void*);


	LogStream& operator<<(float v)
	{
		*this<<static_cast<double>(v);
		return *this;
	}
	LogStream& operator<<(char v)
	{
		buffer_.append(&v,1);
		return *this;
	}
	LogStream& operator<<(const char* str)
	{
		if(str)
			buffer_.append(str,strlen(str));
		else buffer_.append("(null)",6);
		return *this;
	}
	LogStream& operator<<(const unsigned char* str)
	{
		return operator<<(reinterpret_cast<const char*>(str));
	}
	LogStream& operator<<(const std::string& v)
	{
		buffer_.append(v.c_str(),v.size());
		return *this;
	}


	void append(const char* data,int len)
	{
		buffer_.append(data,len);
	}
	const Buffer& buffer()const
	{
		return buffer_;
	}
	void resetBuffer()
	{
		buffer_.reset();
	}


private:
	//void staticCheck();
	template<typename T>
	void formatInteger(T);//整数格式
	Buffer buffer_;//缓冲区
	static const int kMaxNumericSize=32;
};


#endif
