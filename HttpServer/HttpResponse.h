#ifndef _HTTPREPONSE_H_
#define _HTTPREPONSE_H_

/*____________________________________________________________________________
 * 
 * @liu xiangle
 * @Conding And Funning
 *
 * 作用：当网络上的数据到达用户缓冲inbuf，并且解析完成后，
 *       本类负责封装正常响应报文、错误报文
 *
 * 注意：本类没有去操作缓冲区，反而，只是提供了接口、方法，供outbuf缓冲区操作；
 *       Requset类与之相反，因为它内部维护了两个私有成员缓冲区。
 *
 ___________________________________________________________________________*/

#include <string>
#include <map>
#include <memory>
#include "../../BackEnd/include/StudentService.h"


#define CONNECT_TIMEOUT 1000 // 非活跃连接500ms断开
using csk::StudentService;

namespace swings
{
    class Buffer;
    class HttpResponse
    {
    public:
        static const std::map<int,std::string> statusCode2Message;
        static const std::map<std::string,std::string> suffix2Type;

        HttpResponse(int statusCode,std::string path,std::string query,bool keepAlive)
            :statusCode_(statusCode),
            path_(path),
            query_(query),
            keepAlive_(keepAlive){}
        ~HttpResponse(){}
        
        Buffer makeResponse();
        void doErrorResponse( Buffer& output, std::string message );
        void doStaticResponse( Buffer& output, long filesize );

    private:
        std::string _getFileType();
    private:
        int statusCode_;      //状态码
        std::string path_;    //路径
        std::string query_;   // 参数
        bool keepAlive_;      //长连接
        std::shared_ptr<StudentService> student;    
    };
}
#endif
