/*
2021-8-1@liu-yy

1.前后端交互系统 （异步日志的核心）         				 _____________
	前端业务线程数据-------------->前端最下层append接口-—-> |  缓冲系统   |  --->后端最上层threadFunc接口------------->磁盘
				                  		 				  -------------
	虚线长度代表组件逻辑间的距离


2.日志格式：时间 线程id 正文 --源文件:行号
		- 时间：LOG构造时写入
		- 线程id: 和正文一样，在LOG的生命周期内写入
		- 源文件行号：LOG析构时写入
	LOG：匿名对象，生命周期就一行，但是却跑遍了日志库所有的代码，实现了一条消息的封装。


2.异步：进行往磁盘写时，前端业务线程不参与；往后端写时，前端业务线程不用等。
		- 1前后端分离：前端写完后端写，后端写完往磁盘写。业务线程不参与磁盘IO。
		- 2灵活缓冲系统：总共开辟了四个缓冲区，前端那俩在全局区，后端那俩在栈上，
		- 前端写完，交换前后端缓冲区，这样前端不用等后端写完就可以有空间继续写。

3.高性能：
		- 1.异步
		- 2.数据从上游到缓冲系统再到下游的迁移，都是用memcpy()
		- 3.设置标准IO的文件流缓冲区为64K,不满不往磁盘刷，减少IO次数

4.实时性：
		- 1.默认往文件流缓冲每写1024次，刷到磁盘一次。
		- 2.每隔三秒往磁盘刷一次。
*/


#include"../include/AsyncLogging.h"
#include"../include/LogFile.h"
#include<stdio.h>
#include<assert.h>
#include<unistd.h>
#include<functional>

AsyncLogging::AsyncLogging(std::string logFileName_,int flushInterval)
	:flushInterval_(flushInterval),
	 running_(false),
	 basename_(logFileName_),
	 thread_(std::bind(&AsyncLogging::threadFunc,this),"Logging"),//进行了构造
	 mutex_(),//进行了构造
	 cond_(mutex_),//进行了构造
	 latch_(1),
	 currentBuffer_(new Buffer),//shared_ptr
	 nextBuffer_(new Buffer),
	 buffers_()
{
	assert(logFileName_.size()>1);
	currentBuffer_->bzero();//0清空
	nextBuffer_->bzero();
	buffers_.reserve(16);
}


// 前端线程里最靠下的接口，直接与缓冲系统联系。
// 作用：服务于上层函数，把上层交下来的数据复制到缓冲区系统
void AsyncLogging::append(const char* logline,int len)
{
	MutexLockGuard lock(mutex_);
	if(currentBuffer_->avail()>len)//当前缓冲区够写，append追加，这种情况下不push
		currentBuffer_->append(logline,len);
	else//不够写，我们认为这快缓冲区完成使命了，push到数组中;然后使用备用缓冲区进行append
	{
		buffers_.push_back(currentBuffer_);//这里将shared_ptr拷贝了一份放到数组中，引用计数+1了，所以下行reset()没问题。
		currentBuffer_.reset();//当前ccurrentBuffer分离原始指针，下文给他重新赋新的缓冲区
		if(nextBuffer_)
			currentBuffer_=std::move(nextBuffer_);//更新当前缓冲区。
		else
			currentBuffer_.reset(new Buffer);//若这个时候nextbunffer空，那就在new4M，引用计数将再次变为1。//很少发生的情况，需要拿后端来交换
		currentBuffer_->append(logline,len);//append追加
		cond_.notify();// 通知日志线程，有数据可写
	}
}



// 后端线程里最靠上的接口，直接与缓冲系统联系。
// 作用：往下分散任务，进行内存到磁盘IO。
void AsyncLogging::threadFunc()
{
	assert((running_==1));
	latch_.countDown(); //latch_-1并唤醒前端线程。
	LogFile output(basename_); //【这里实例化了一个LogFile,并将日志文件名传给了这个对象，主要功能是提供一个每过若干次写1024入就自动执行一次flush的功能。】

	BufferPtr newBuffer1(new Buffer); //后台线程的两个缓冲4M
	BufferPtr newBuffer2(new Buffer);
	newBuffer1->bzero();
	newBuffer2->bzero();
	BufferVector buffersToWrite; //后台的就绪队列，用来和前台线程的buffers_进行swap
	buffersToWrite.reserve(16);

	while(running_)
	{
		assert(newBuffer1&&newBuffer1->length()==0);
		assert(newBuffer2&&newBuffer2->length()==0);
		assert(buffersToWrite.empty());
		{
			MutexLockGuard lock(mutex_);
			if(buffers_.empty())
				cond_.waitForSeconds(flushInterval_);//如果buffer为空，那么表示没有数据需要写入文件，那么就等待指定的时间,不一直等（注意这里没有用倒数计数器）
			buffers_.push_back(currentBuffer_);
			//这一行，两种情况，但是最后一定是当前currentbuffer没写满，就给放到队列中。因为我后台既然写，就一定要把当前前台发来的数据全部写到磁盘。
			//1.时间没到，但前台给任务了，被唤醒 。注意，这种情况下，currentbuffer已经是下一个buffer了，已经又被写入了，因为前台唤醒到后台开始写有时间间隔。
			//2.时间到了，前台写的太慢，没写满也得压入队列
			currentBuffer_.reset();

			currentBuffer_=std::move(newBuffer1);//更新当前缓冲区
			buffersToWrite.swap(buffers_);//实现异步的核心代码，交换前后台队列。
			if(!nextBuffer_)//呼应前台线程那一行了
				nextBuffer_=std::move(newBuffer2);

		}

		assert(!buffersToWrite.empty());
		if(buffersToWrite.size()>25)//大于25很正常吧，只要你前端线程写的足够快就行呗，vector又自动扩容，多好，用的时候基本不用担心越界的事儿。
			buffersToWrite.erase(buffersToWrite.begin()+2,buffersToWrite.end());// 丢掉多余日志，以腾出内存，仅保留两块缓冲区
		for(size_t i=0;i<buffersToWrite.size();++i)
			output.append(buffersToWrite[i]->data(),buffersToWrite[i]->length());
		if(buffersToWrite.size()>2)//为什么可能会>2呢？？因为一开始可能没有>25没有置为2啊
			buffersToWrite.resize(2);//为什么置为2？ 因为我就有俩缓冲区，数组里智能指针多了没用，少了不行。


		//更新俩缓冲区
		if(!newBuffer1)
		{
			assert(!buffersToWrite.empty());
			newBuffer1=buffersToWrite.back();//这里发生了拷贝，引用计数+1了，所以下一行立刻出队使得数组里的智能指针析构，引用计数再-1
			buffersToWrite.pop_back();//这样就使得，底层缓冲区还是那个缓冲区，管理他的智能指针又换回最开始的那个。
			newBuffer1->reset();//这个是清空缓冲区，FixedBuffer里封装的，不是智能指针里的那个reset()
		}
		if(!newBuffer2)
		{
			assert(!buffersToWrite.empty());
			newBuffer2=buffersToWrite.back();
			buffersToWrite.pop_back();
			newBuffer2->reset();
		}
		buffersToWrite.clear();
		output.flush();
	}
	output.flush();
}





