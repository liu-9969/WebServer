/**************************************************************************************
 * @liu xiangle
 * coding and funning
 *
 * 作用：本类主要实现的是把网络上的数据读到inbuff缓冲区，然后解析;
 *       把解析的结果放在了各个成员变量中，或者说把读缓冲区的数据拷贝到各个成员变量中。
 *
 * 注意：本类是直接操作了读写两个缓冲区对象
 *       关于定时器，本类中没有去实现安装，是在_acceptConnection()中实现的
 *
 * 接口：向外提供以read()、write() 、parseRequest()为主public成员函数
 *************************************************************************************/


#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_

#include "Buffer.h"
#include <string>
#include <map>

namespace swings
{
    class Timer;
    class HttpRequest
    {
        public:
            //解析状态、请求方法、http版本
            enum HttpRequestParseState  {
                ExpectRequestLine,
                ExpectHeaders,
                ExpectBody,
                GotAll
            };
            enum Method{
                Invalid,Get,
                Post,Head,
                Put,Delete
            };
            enum Version{
                Unknown,
                HTTP10,
                HTTP11
            };


        public:
            HttpRequest(int fd)
               :fd_(fd),
                working_(false),
                timer_(nullptr),
                state_(ExpectRequestLine),
                method_(Invalid),
                version_(Unknown)
            {assert(fd_ >= 0);}
            ~HttpRequest();//close(fd_);

            //核心的三个函数
            int read(int* saveErrno);     //读网络数据
            int write(int * saveErrno);   //写给网络数据
            bool parseRequest();          //解析http报文

            //类外修改私有成员的接口
            void appendOutBuffer(const Buffer& buf) {outBuff_.append(buf);}
            int writeableBytes() {return outBuff_.readableBytes();}
            void setWorking(){working_ = true;}
            void setNoWorking(){working_ = false;}
            void setTimer(Timer* timer){timer_ = timer;}

            //类外访问私有成员的接口
            int fd() {return fd_;}
            Timer* getTimer(){return timer_;}
            bool parseFinish() {return state_ == GotAll;}
            bool isWorking()const {return working_;}
            std::string getPath()const {return path_;}
            std::string getQuery()const {return query_;}
            std::string getHeader(const std::string& filed)const;
            std::string getMethod()const;
            void resetParse();              //重置解析状态
            bool keepAlive()const;          //是否长连接
            


        private:
            //类内部函数--不供外使用
            bool _parseRequestLine(const char* begin,const char* end);           //解析请求行
            bool _setMethod(const char* begin,const char* end);                  //设置请求方法
            void _setPath(const char* begin,const char* end);                    //设置URL路径
            void _setQuery(const char* begin,const char* end);                   //设置URL参数
            void _setVersion(Version version);                  //设置http版本
            void _addHeader(const char* begin,const char* colon,const char* end);//增加报头


        private:
            //1.缓冲区相关的
            int fd_;                      //文件描述符
            Buffer inBuff_;                //读缓冲
            Buffer outBuff_;               //写缓冲
            bool working_;                 //若正在工作，则不能被超时断开

            //2.定时器相关的
            Timer* timer_;

            //报文解析相关的
            HttpRequestParseState state_; //状态
            Method method_ ;//方法
            Version version_;             //版本
            std::string path_;            //路径
            std::string query_;           //参数
            std::map<std::string,std::string> headers_;  //首部行头，字段

    };
}

#endif



