##### 1.堆内存释放错误

​	

bug问题：

```
问题描述： double free or corruption (fasttop)   free():  
		 double free detected in tcache 2
		 
 bug所在：Timer.cpp::handleTimer()中，while循环条件本应为!timerQueue_.empty();误写为					  timerQueue_.size()

1. 找bug位置：
一开始遇到这个问题，很奇怪，有时候webbench测试5000客户端，60s内不出问题，有时候本地一个浏览器崩掉。。。
然后上网查了下，无非就是二次释放或内存越界造成的。然后就不知道从哪下手了。。。理了很多遍逻辑，其实只有两个类的实例会delete，像epoll模型，线程池，定时器管理器，他们只有一个实例，程序运行期间永远不会delete;Buffer、HttpResponse没有在堆上开辟空间。所以这些我都不用考虑了，只有HttpRequest和Timer我需要考虑,这样问题就是出在了连接上，定位好这俩类的delete所在函数，进行打断点单步调试，最后在hangleTimer里边出现 double free detected in tcache 2。到这我确定了是timer的delete出问题了。

2. 改正错误
接下来我去找怎么“二次释放”的，瞪着眼看这个函数看了很多遍，又理了很多遍逻辑，都没啥问题，gdb我也之只能知道是timer出问题了，我开始考虑是不是内存溢出了，这个函数访问了那个计时器队列，我想是不是访问过头了（其实队列不会溢出，因为只能通过对头出队来访问，不像数组，可以通过下标）。我就开始看循环条件，就是在这，我发现了重大问题，我的循环条件竟然是!timerQueue_.size(),瞬间感觉不对劲。因为我记得当时写的时候它和getNextExprieTime的循环条件一样，立马去对比，果然。。。。。。。
这个条件确实错了，这样这个队列不能正确被处理，改正后，重新测试即便都没有问题。

3.思考为什么二次释放了
我开始冷静思考，为啥这个bug会时而出来，时而不出来。为啥就二次释放了。如下：
主控线程：读事件就绪->handleTimer无效->3getNextExprieTime->处理写->4handleTimer出错
    ​		|
    		|
    		+
 IO线程：  线程池中：a首先关闭timer

​		情况1. 若a比3快，3处就释放掉了timer，4处正好队列空，循环条件满足，程序要再次释放队头元素
			  但是，此时队列是空的，但是底层数组还没有抹掉这个值，size等于0只是不让我访问，那我硬要访问它也				 拦不住啊，于是导致释放错误。

​		情况2. 若3比a快，3处不会释放掉timer ，4处不空，循环条件不满足，本次连接没问题

​		但是，所有的连接都是这么个情况，肯定会出现情况1，出现就挂。





```

逻辑简化：

![image-20210603153841361](C:\Users\liu-cc\AppData\Roaming\Typora\typora-user-images\image-20210603153841361.png)

![image-20210603153925365](C:\Users\liu-cc\AppData\Roaming\Typora\typora-user-images\image-20210603153925365.png)

![image-20210603110813038](C:\Users\liu-cc\AppData\Roaming\Typora\typora-user-images\image-20210603110813038.png)



2.死锁：





