
#include"LogStream.h"
#include<algorithm>
#include<limits>
#include<stdint.h>
#include<stdio.h>



/*
    Matthew Wilson的《Efficient Integer to String Conversions》
    功能：高效整数转字符串，并通过传出参数传出，返回字符串长度strlen
    （ int、unsigned int、long、unsigned long、long long、unsigned long long ）
    算法思路：先从个位向高位取出每位上的数，放到数组buf[]中，复数在最后加"-",然后加'\0',最后反转这个字符串，
    思考：sprintf()效率很差嘛？
*/
const char digits[] = "9876543210123456789";
const char* zero = digits + 9;//zero初始指向0
template<typename T>
size_t convert(char buf[],T value)
{
	T i=value;
	char*p=buf;
	do
	{
		int lsd=static_cast<int>(i%10);
		i/=10;
		*p++=zero[lsd];
	}while(i!=0);         //循环结束后，p指向buf第一个待写的位置

	//为复数添加-号
	if(value<0)
		*p++='-';        //p依旧指向buf第一个空的位置
	*p='\0';             //此时，这个空的位置填上了字符串结束符号

	//将字符串逆转过来
	std::reverse(buf,p); //注意：这个函数是反转[buf,p-1]的内容。
	return p-buf;        //返回字符串的长度，不算'\0'
}







template class FixedBuffer<kSmallBuffer>;//声明两个模板类
template class FixedBuffer<kLargeBuffer>;

template<typename T>
void LogStream::formatInteger(T v)
{
	if(buffer_.avail()>=kMaxNumericSize)//可用的长度
	{
		size_t len=convert(buffer_.current(),v);//在FixedBuffer外部向buffer_写入数据
		buffer_.add(len);//更新cur指针的位置
	}
}






//整数转字符串，并同时写到FixedBuffer buffer_
LogStream& LogStream::operator<<(short v)
{
	*this<<static_cast<int>(v);
	return *this;
}
LogStream& LogStream::operator<<(unsigned short v)
{
	*this<<static_cast<unsigned int>(v);
	return *this;
}
LogStream& LogStream:: operator<<(int v)
{
	formatInteger(v);
	return *this;
}
LogStream& LogStream::operator<<(unsigned int v)
{
	formatInteger(v);
	return *this;
}
LogStream& LogStream::operator<<(long v)
{
	formatInteger(v);
	return *this;
}
LogStream& LogStream::operator<<(unsigned long v)
{
	formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(long long v)
{
	formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned long long v)
{
	formatInteger(v);
	return *this;
}




//浮点型转字符串，并同时写到缓冲区FixedBuffer buffer_
LogStream& LogStream::operator<<(double v)
{
	if(buffer_.avail()>=kMaxNumericSize)
	{
		int len=snprintf(buffer_.current(),kMaxNumericSize,"%.12g",v);
		buffer_.add(len);
	}
	return *this;
}
LogStream& LogStream::operator<<(long double v)
{
	if(buffer_.avail()>=kMaxNumericSize)
	{
		int len=snprintf(buffer_.current(),kMaxNumericSize,"%.12Lg",v);
		buffer_.add(len);
	}
	return *this;
}

