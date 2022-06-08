//@liu xiangle
//coding And Funning

#include <unistd.h>
#include <algorithm>
#include <iostream>
#include "../include/HttpRequest.h"
//#define STATIC_ROOT "~/Desktop"

using namespace swings;
using std::string;
using std::find;
using std::equal;



HttpRequest::~HttpRequest()
{
    close(fd_);
}



//从网络套接字中读到inbuf缓冲区
int HttpRequest::read(int* saveErrno)
{
    int ret = inBuff_.readFd(fd_,saveErrno);
    return ret;
}
//从outbuf中写给网络套接字
int HttpRequest::write(int* saveErrno)
{
    int ret = outBuff_.writeFd(fd_,saveErrno);
    return ret;
}





/********************************************************
 *说明：
 *  crlf：指向每一行的“\r\n”中的\r;
 *  crlf+2：向后偏移两个元素到下一行
 *  循环条件：有没有解析完
 *
 *  const修饰函数返回值：
 *  指针：指向的内容不能变
 *  引用：指向和指向的内容都不能变
 *
 *  返回值：正常完：true  
*********************************************************/
bool HttpRequest::parseRequest()
{
    bool ok = true;
    bool hasMore = true;
    while(hasMore)
    {
        //解析请求行
        if(state_ == ExpectRequestLine)
        {
            const char* crlf = inBuff_.findCRLF();//找到返回‘\r’的地址，否则返回nullptr
            if(crlf)
            {
                ok = _parseRequestLine(inBuff_.peek(),crlf);
                if(ok)
                {
                    inBuff_.retrieveUntil(crlf + 2);//crlf指向“\r\n”中的\r，+2表示向后偏移两个字节到下一行起始位置
                    state_ = ExpectHeaders;
                }
                else
                    hasMore = false;
            }
            else
                hasMore = false;
        }

        //解析请求头
        if(state_ == ExpectHeaders)
        {
            const char* crlf = inBuff_.findCRLF();
            if(crlf)
            {
                const char* colon = find(inBuff_.peek(),crlf,':');
                if(colon != crlf)
                    _addHeader(inBuff_.peek(),colon,crlf);
                else //已经解析到最后一行，就是那个只有“\r\n”的空行
                {
                    state_ = GotAll;
                    hasMore = false;
                }     
                inBuff_.retrieveUntil(crlf + 2); //解析完一行就偏移到下一行，包括那个空行，完事也得偏移
            }
            else{
                hasMore = false;}
        }
        //解析包体
        if(state_ == ExpectBody)
        {}
    }
    return ok;
}




//思路：解析请求行（按照格式' '去解析这一段缓冲）
//参数：1.报文(缓冲区)首地址   2.“\r\n”中'\r'的地址
//返回：成功：true  失败：false

bool HttpRequest::_parseRequestLine(const char* begin,const char* end)
{
    bool succeed = false;
    const char* start = begin;
    const char* space = find(start,end,' '); //解析方法
    if(space != end && _setMethod(start,space))
    {
        start = space + 2;
        space = find(start,end,' ');//解析路径
        if(space != end)
        {
            const char* question = find(start,space,'?');
            if(question != space)
            {
                _setPath(start,question);
                _setQuery(question,space);
            }
            else
                _setPath(start, space);
        
            start = space + 1;//解析版本
            //string m(start,end),字符串最后一个字符为*（end-1）
            succeed = end - start == 8 && equal(start,end-1,"HTTP/1.");
            if(succeed)
            {
                if(*(end - 1 ) == '1')
                    _setVersion(HTTP11);
                else if(*(end - 1) == '0')
                    _setVersion(HTTP10);
                else
                    succeed = false;
            }
        }
    }
    return succeed;
}




//第一个参数:字符串首地址； 第二个参数：迭代器，指向方法后的空格，find返回值
bool HttpRequest::_setMethod(const char*begin,const char* end)
{
    string m(begin,end);
    //std::cout<<"请求方法："<<m<<std::endl;
    if(m == "GET")
        method_ = Get;
    else if(m == "POST")
        method_ = Post;
    else if(m == "HEAD")
        method_ = Head;
    else if(m == "PUT")
        method_ = Put;
    else if(m == "DELETE")
        method_ = Delete;
    else
        method_ = Invalid;

    return method_ != Invalid; 
}




void HttpRequest::_setQuery(const char* begin,const char* end)
{
    query_.assign(begin,end);
}




void HttpRequest::_setVersion(Version version)
{
    version_ = version;
    //std::cout<<"协议版本："<<version_<<std::endl;
}




void HttpRequest::_setPath(const char* begin,const char* end)
{
    string subPath;
    subPath.assign(begin,end);
    //std::cout<<"路径："<<subPath<<std::endl;
    if(subPath == "/")//请求没有指定路径
        subPath = "index.html";//给他一个默认路径
    path_ = subPath;
    //std::cout<<"请求路径："<<path_<<std::endl;
}




//向map里添加首部行，key:字段 value:内容
void HttpRequest::_addHeader(const char* begin,const char* colon,const char* end)
{
    string filed(begin,colon);//首部字段
    ++ colon;
    while(colon < end && *colon == ' ')
        ++ colon;
    string value(colon,end);
    while(!value.empty() && value[value.size()-1] == ' ')//首部行的最后一行
        value.resize(value.size()-1);//把最后一行的空格删掉
    headers_[filed] = value;
}





//获取方法
string HttpRequest::getMethod() const
{
    string res;
    if(method_ == Get)
        res = "GET";
    else if(method_ == Post)
        res = "POST";
    else if(method_ == Head)
        res = "HEAD";
    else if(method_ == Put)
        res = "Put";
    else if(method_ == Delete)
        res = "DELETE";
    
    return res;
}




//获取首部内容，依据首部字段查找
string HttpRequest::getHeader(const string& field) const
{
    string res;
    auto itr = headers_.find(field);
    if(itr != headers_.end())
        res = itr -> second;
    return res;
}




//重置
void HttpRequest::resetParse()
{
    state_ = ExpectRequestLine; // 报文解析状态
    method_ = Invalid; // HTTP方法
    version_ = Unknown; // HTTP版本
    path_ = ""; // URL路径
    query_ = ""; // URL参数
    headers_.clear(); // 报文头部
}




//长连接
bool HttpRequest::keepAlive() const
{
    std::string connection = getHeader("Connection");
    bool res = connection == "Keep-Alive" ||
        (version_ == HTTP11 && connection != "close");

    return res;
}




