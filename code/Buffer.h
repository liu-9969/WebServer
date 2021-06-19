//@liu xiangle
//2021-5-13

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <vector>
#include <assert.h>
#include <string>
#include <algorithm>
#define INIT_SIZE 1024


/*@code

     +------------------------------------------------------------------------+
     |    perpendable     |      readable bytes      |    writeable bytes    |
     |                    |         (content)        |                       |
     +------------------------------------------------------------------------+
     |                    |                          |
     0        <=     readerIndex        <=       writerIndex      <=     size()


@endcode*/




namespace  swings
{

    class Buffer
    {
    public:
        Buffer()
            :buffer_(INIT_SIZE),
             readerIndex_(0),
             writerIndex_(0)
        {
            assert(readableBytes() == 0 );
            assert(writeableBytes() == INIT_SIZE);
        }
        ~Buffer(){}

    public:
        //返回可读、可写、缓冲区前空闲字节数    
        //返回可写位置
        //返回可读位置
        //写入数据后，移动writerIndex
        ssize_t readFd(int fd,int* saveErrno);
        ssize_t writeFd(int fd,int* saveErrno);
        
        size_t readableBytes() const             
        {return writerIndex_ - readerIndex_;}

        size_t writeableBytes() const            
        {return buffer_.size() - writerIndex_;}

        size_t prependableBytes() const          
        {return readerIndex_;}

        char* beginWrite()                       
        {return _begin() + writerIndex_;}

        const char* beginWrite() const           
        {return _begin() + writerIndex_;}

        const char* peek() const                 
        {return _begin() + readerIndex_;}

        void append(const void* data,size_t len) 
        {append(static_cast<const char*>(data),len);}

        void append(const Buffer& other)         
        {append(other.peek(),other.readableBytes());}

        void append(const std::string& str) // 插入数据
        { append(str.data(), str.length()); }

        void hasWriten(size_t len)               
        {writerIndex_ += len;}




        //读取数据后，readIndex移动len个字节
        void retrieve(size_t len)
        {
            assert(len <= readableBytes());
            if(len < readableBytes()){
                readerIndex_ += len;
            }
            else{
                retrieveAll();
            }
        }
        //读至end
        void retrieveUntil(const char* end)
        {
            assert(end >= peek());
            assert(end <= beginWrite());
            retrieve(end - peek());
        }
        //初始化
        void retrieveAll()
        {
            readerIndex_ = 0;
            writerIndex_ = 0;
        }
        //以string方式读取全部内容
        std::string retrieveAllAsString()
        {return retrieveAsString(readableBytes());}
        //以string方式读取len个字节
        std::string retrieveAsString(size_t len)
        {
            assert(len <= readableBytes());
            std::string result(peek(),len);
            retrieve(len);
            return result;
        }





        //确保writeable Bytes有足够空间
        void ensureWriteableBytes(size_t len)
        {
            if(len > writeableBytes()){
                _makeSpace(len);
            }
            assert(writeableBytes() >= len);
        }
        //插入数据
        void append(const char* data,size_t len)
        {
            ensureWriteableBytes(len);
            std::copy(data,data+len,beginWrite());
            hasWriten(len);
        }

        //查找"\r\n"
        const char* findCRLF() const
        {
            const char CRLF[] = "\r\n";
            const char* crlf = std::search(peek(), beginWrite(), CRLF, CRLF+2);
            return crlf == beginWrite() ? nullptr : crlf;
        }
        const char* findCRLF(const char* start) const
        {
            assert(peek() <= start);
            assert(start <= beginWrite());
            const char CRLF[] = "\r\n";
            const char* crlf = std::search(start, beginWrite(), CRLF, CRLF + 2);
            return crlf == beginWrite() ? nullptr : crlf;
        }



    private:
        //返回缓冲区开始的指针
        char* _begin() {return &*buffer_.begin();}
        const char* _begin()const {return &*buffer_.begin();}
        //确保writeable Bytes有足够的空间
        void _makeSpace(size_t len)
        {
            size_t readable = readableBytes();
            if(len >= writeableBytes() + prependableBytes()){
                buffer_.resize(writerIndex_ + len);
            }
            else{
                //FIXME:move bytes to prepend
                std::copy(
                          _begin() + readerIndex_,//peek()不可以，因为peek()返回值被const修饰了
                          _begin() + writerIndex_,
                          _begin());
                readerIndex_ = 0;
                writerIndex_ = readable;
                assert(readable == readableBytes());//是否全部拷贝到头部
            }
        }

    private:
        std::vector<char> buffer_;
        size_t readerIndex_;
        size_t writerIndex_;

};
}

#endif
