#include "HttpResponse.h"
#include "Buffer.h"

#include <fcntl.h>//open
#include <sys/mman.h>//mmap munmap
#include <unistd.h>// close
#include <sys/stat.h>//stat

using namespace swings;
using std::string;
using std::map;

const std::map<int, string> HttpResponse::statusCode2Message =
{
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"}
};




const std::map<string, string> HttpResponse::suffix2Type = 
{
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".mp3", "audio/mp3"},
    {".mov", "video/quicktime"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css"}
};




Buffer HttpResponse::makeResponse()
{
    Buffer output;
    if(statusCode_ == 400)
    {
        doErrorResponse(output,"Boy_liu can't prase the file");
        //printf("封装400\n");
        return output;
    }
    struct stat sbuf;
    // 文件找不到错误404
    if(::stat(path_.data(),&sbuf) < 0)
    {
        statusCode_ = 404;
        doErrorResponse(output,"Boy_liu can't find the file");
        //printf("封装404\n");
        return output;
    }
    // 权限错误403
    if(!(S_ISREG(sbuf.st_mode) || !(S_IRUSR & sbuf.st_mode))) {
        statusCode_ = 403;
        doErrorResponse(output, "Swings can't read the file");
        //printf("封装403\n");
        return output;
    }
    // 处理静态请求
    //printf("封装响应报文\n");
    doStaticResponse(output,sbuf.st_size);
    return output;
}





void HttpResponse::doStaticResponse( Buffer &output, long filesize )
{
    assert(filesize >= 0);
    auto itr = statusCode2Message.find(statusCode_);
    if(itr == statusCode2Message.end())
    {
        statusCode_ = 404;
        return;
    }
    // 响应行
    output.append("HTTP/1.1 " + std::to_string(statusCode_) + " " + itr -> second + "\r\n");
    // 报头
    if(keepAlive_)
    {
        output.append("Connection: Keep-Alive\r\n");
        output.append("Keep-Alive: timeout=" + std::to_string(CONNECT_TIMEOUT) + "\r\n");
    }
    else 
        output.append("Connection: close\r\n");
    output.append("Content-type: " + _getFileType() + "\r\n");
    output.append("Content-length: " + std::to_string(filesize) + "\r\n" );
    // 添加头部Last-Modified: ?
    output.append("Server: Boy_Liu\r\n");
    output.append("\r\n");
    // 报文体
    int srcFd = ::open(path_.data(),O_RDONLY,0);
    void* mmapRet = ::mmap(NULL,filesize,PROT_READ,MAP_PRIVATE,srcFd,0);
    ::close(srcFd);
    if(mmapRet == (void*)-1)
    {
        munmap(mmapRet,filesize);
        output.retrieveAll();
        statusCode_ = 404;
        doErrorResponse(output,"Boy_liu can't find the file");
        return;
    }
    char* srcAddr = static_cast<char*>(mmapRet);
    output.append(srcAddr,filesize);
    munmap(srcAddr,filesize);
}




// 封装错误的应答协议
void HttpResponse::doErrorResponse( Buffer& output,string message )
{
    string body;
    auto itr = statusCode2Message.find(statusCode_);
    if(itr == statusCode2Message.end())
        return ;
    body += "<html><title>哎～出错了</title>";
    body += "<body bgcolor=\"ffffff\">";
    body += "<div align=center>" + std::to_string(statusCode_) + " : " + itr -> second + "\n";
    body += "<p>" + message + "</p> + </div>";
    body += "<hr size=3 /><em>Liu xiangle's Web Server</em></body></html>";

    // 响应行
    output.append("HTTP/1.1 " + std::to_string(statusCode_) + " " + itr -> second + "\r\n");
    // 报文头首部行
    output.append("Server: Boy_Liu\r\n");
    output.append("Content-type: text/html\r\n");
    output.append("Connection: close\r\n");
    output.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    // 报文体实体
    output.append(body);
}


// 本地文件类型-->网络文件类型
string HttpResponse:: _getFileType()
{
    size_t idx = path_.find_last_of('.');
    string suffix;
    if(idx == std::string::npos)
        return "text/plain";
    suffix = path_.substr(idx);
    auto ret = suffix2Type.find(suffix);
    if(ret == suffix2Type.end())
        return "text/plain";
    return ret -> second;
}
