
/**************************************************************************
 * 创建时间：2021-3-26
 * 修改时间：2021-3-27
 * 程序：线程池
 *
 * ************************************************************************/

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#define MIN_WAIT_TASK_NUM 10      //
#define DEFAULT_THREAD_VARY 10    //每次创建和销毁线程的数量
#define DEFAULT_TIME 10
#define true 1
#define false 0







 /*各个子线程任务的结构体*/
typedef struct {
    void* (*function)(void*);             //函数指针，回调函数
    void* arg;                            //上面函数的参数
}threadpool_task;

/*描述线程池相关信息的结构体，用这样一个结构体来实现一个线程池*/
typedef struct pthreadpool_t
{
    pthread_mutex_t lock;                 //本结构体的互斥锁
    pthread_mutex_t thread_counter;       //记录忙状态线程个数的互斥锁
    pthread_cond_t queue_not_full;        //当任务队列满时，添加任务的线程阻塞，等待此条件变量
    pthread_cond_t queue_not_empty;       //当任务队列不为空时，唤醒等待该条件变量的线程

    pthread_t* pthreads;                  //线程tid数组
    pthread_t adjust;                     //管理线程的tid;
    threadpool_task* task_queue;          //任务队列

    int min_thr_num;                      //线程池中最少线程数
    int max_thr_num;                      //线程池中最多线程个数
    int live_thr_num;                     //线程池中存活的或已有的线程
    int busy_thr_num;                     //线程中当前忙线程的个数
    int wait_exit_thr_num;                //要销毁的线程个数

    int queue_front;                      //队头下标
    int queue_rear;                       //队尾下标
    int queue_size;                       //队列中实际存在的任务数
    int queue_max_size;                   //队列中可容纳最大的任务个数

    int shutdown;                         //标志位，线程池的使用状态，false/truee
}threadpool_t;









/*声明一些函数*/
int is_thread_alive(pthread_t tid);        //线程是否存在
int threadpool_free(threadpool_t* pool);   //销毁锁、条件变量；释放线程池占用的内存





/*工作线程*/
void* work_pthread(void* threadpool)
{
    threadpool_t* pool = (threadpool_t*)threadpool;
    threadpool_task task;

    //先对这个线程池加锁，期间不让别的线程操作
    pthread_mutex_lock(&pool->lock);

    while (true)
    {
        while (pool->queue_size == 0 && pool->shutdown == false)
        {
            //当前没有任务可做的话，等待，阻塞在not_empty上。
            printf("thread %ld is waiting\n", pthread_self());
            pthread_cond_wait(&pool->queue_not_empty, &pool->lock);//-----------------------------------------------------------------------
            //刚有任务，不需要太多线程，清理一些
            if (pool->wait_exit_thr_num > 0) {
                pool->wait_exit_thr_num--;
                //如果当前生存的线程 > Min,关闭当前线程
                if (pool->live_thr_num > pool->min_thr_num) {
                    printf("exit pthread %ld \n", pthread_self());
                    pool->live_thr_num--;
                    pthread_mutex_unlock(&pool->lock);
                    pthread_exit(NULL);
                }
            }
        }

        //指定了true
        if (pool->shutdown) {
            printf("pthread 0x%x is exited\n", (unsigned int)pthread_self());
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }

        //有任务了，取任务;出队操作
        task.function = pool->task_queue[pool->queue_front].function;
        task.arg = pool->task_queue[pool->queue_front].arg;
        pool->queue_front = (pool->queue_front + 1) % pool->queue_max_size;
        pool->queue_size--;

        //通知可以有新任务加进来了,唤醒阻塞在not_full上的线程
        pthread_cond_broadcast(&(pool->queue_not_full));

        //任务取出后，立即解锁
        pthread_mutex_unlock(&pool->lock);

        //执行任务,busy_thr_num ++
        printf("pthread %ld start working\n",pthread_self());
        pthread_mutex_lock(&pool->thread_counter);                                //忙状态线程数加锁
        pool->busy_thr_num++;                                                    //忙状态线程数+1
        pthread_mutex_unlock(&pool->thread_counter);                              //忙状态线程数解索
        (*(task.function))(task.arg);                                             //调用回调函数，执行任务

        //任务结束，busy_thr_num --
        printf("pthread %ld end working\n",pthread_self());
        pthread_mutex_lock(&(pool->thread_counter));                              //忙状态线程数加锁
        pool->busy_thr_num--;                                                    //忙状态线程数 -1
        pthread_mutex_unlock(&(pool->thread_counter));                            //忙状态线程数解索

    }
    return 0;
}






/*管理线程*/
void* adjust_pthread(void* threadpool)
{
    int i;
    threadpool_t* pool = (threadpool_t*)threadpool;
    sleep(DEFAULT_TIME);                                                         //定时管理线程池

    pthread_mutex_lock(&(pool->lock));                                           //加锁--访问线程池成员--解锁
    int queue_size = pool->queue_size;
    int live_thr_num = pool->live_thr_num;
    pthread_mutex_unlock(&(pool->lock));

    pthread_mutex_lock(&(pool->thread_counter));                                 //加锁--访问结构体成员--解锁
    int busy_thr_num = pool->busy_thr_num;
    pthread_mutex_lock(&(pool->thread_counter));

    /*创建新线程的算法：任务数 > 最小线程个数 &并且&  已存在线程 < 最大线程个数*/
    if (queue_size >= MIN_WAIT_TASK_NUM && live_thr_num < pool->max_thr_num) {
        printf("1111\n");
        pthread_mutex_lock(&(pool->lock));
        int add = 0;

        for (i = 0;                                                          //一次增加DEFAULT_THREAD_VARY个线程
            i < pool->max_thr_num &&
            add < DEFAULT_THREAD_VARY &&
            pool->live_thr_num < pool->max_thr_num;
            i++)
        {
            printf("1111\n");
            if (pool->pthreads[i] == 0 || !is_thread_alive(pool->pthreads[i])){
                pthread_create(&(pool->pthreads[i]), NULL, work_pthread, (void*)pool);
                printf("creat thread %ld\n",pool->pthreads[i]);
                add++;
                pool->live_thr_num++;
            }
        }  
        pthread_mutex_unlock(&(pool->lock));
    }

    /*销毁多余线程的算法：忙线程个数x2 < 存活的线程数，且 存活线程数 > 最小线程数*/
    if (busy_thr_num * 2 < live_thr_num && live_thr_num > pool->min_thr_num)
    {
        pthread_mutex_lock(&(pool->lock));
        pool->wait_exit_thr_num = DEFAULT_THREAD_VARY; //默认销毁10个
        pthread_mutex_unlock(&(pool->lock));

        for (i = 0; i < DEFAULT_THREAD_VARY; i++) {
            pthread_cond_signal(&(pool->queue_not_empty));//通知处在空闲状态的线程，他们会自行终止
        }
    }

    return NULL;
}













/*创建线程池，初始化threadpool_t结构体成员*/
threadpool_t* threadpool_creat(int min_thr_num, int max_thr_num, int queue_max_size)
{
    int i;
    threadpool_t* pool = NULL;
    do {
        /*给线程池分配空间*/
        if ((pool = (threadpool_t*)malloc(sizeof(threadpool_t))) == NULL) {
            printf("malloc threadpool error\n");
            break;
        }

        /*初始化结构体成员*/
        pool->min_thr_num = min_thr_num;
        pool->max_thr_num = max_thr_num;
        pool->busy_thr_num = 0;
        pool->live_thr_num = min_thr_num;
        pool->queue_size = 0;
        pool->queue_front = 0;
        pool->queue_rear = 0;
        pool->shutdown = false;
        pool->queue_max_size = queue_max_size;

        /*给线程tid数组分配空间*/
        if ((pool->pthreads = (pthread_t*)malloc(sizeof(pthread_t) * pool->max_thr_num)) == NULL) {
            printf("pthreads malloc error\n");
            break;
        }
        memset(pool->pthreads, 0, sizeof(pool->pthreads) * max_thr_num);

        /*给任务队列分配空间*/
        if ((pool->task_queue = (threadpool_task*)malloc(sizeof(threadpool_task) * queue_max_size)) == NULL) {
            printf("task_queue malloc error\n");
            break;
        }
        memset(pool->task_queue, 0, sizeof(pool->task_queue) * queue_max_size);

        /*初始化互斥锁，条件变量*/
        if (pthread_mutex_init(&pool->lock, NULL) != 0
            || pthread_mutex_init(&pool->thread_counter, NULL) != 0
            || pthread_cond_init(&pool->queue_not_full, NULL) != 0
            || pthread_cond_init(&pool->queue_not_empty, NULL) != 0) {
            printf("pthread init error\n");
            break;
        }

        /*创建启动最少个线程，管理线程*/
        for (i = 0; i < pool->min_thr_num; i++) {
            pthread_create(&(pool->pthreads[i]), NULL, work_pthread, (void*)pool);
            printf("creat thread %ld\n",pool->pthreads[i]);
        }
        pthread_create(&(pool->adjust), NULL, adjust_pthread, (void*)pool);

        return pool;
    } while (0);
    threadpool_free(pool);
    return NULL;
}










/*向线程池中的任务队列添加任务*/
int thread_add(threadpool_t* pool, void* (*func)(void* arg), void* arg)
{
     //pthread_mutex_lock(&(pool->lock));
     //printf("0000\n");
     //printf("queue_size:%d\n",pool->queue_size);
     //printf("queue_max_size:%d\n",pool->queue_max_size);
     //printf("shutdown:%d\n",pool->shutdown);
    //当任务队列满时，阻塞等待在not_full这个条件变量上，被唤醒后在添加任务，
    //正式添加任务前加锁，不允许别的线程去操作这个线程池结构体
    if ((pool->queue_size == pool->queue_max_size) && (pool->shutdown == false)) {
        pthread_cond_wait(&(pool->queue_not_full), &(pool->lock));   //阻塞在该条件变量上，等待该条件满足时，被唤醒（阻塞，自动解索，被唤醒后在尝试枷锁）
    }
//printf("1111\n");
    //清空工作线程回调函数的参数
    if (pool->task_queue[pool->queue_rear].arg != NULL) {
        free(pool->task_queue[pool->queue_rear].arg);
        pool->task_queue[pool->queue_rear].arg = NULL;
    }

    //添加任务到队列中
    pool->task_queue[pool->queue_rear].function = func;
    pool->task_queue[pool->queue_rear].arg = arg;
    pool->queue_rear = (pool->queue_rear + 1) % pool->queue_max_size;   //队尾指针移动，模拟的
    pool->queue_size++;

    //唤醒阻塞在not_empty上的线程
    pthread_cond_signal(&pool->queue_not_empty);
    pthread_mutex_unlock(&(pool->lock));                                //解锁

    return 0;
}




/*线程池销毁，回收所有的线程*/
int threadpool_destroy(threadpool_t* pool)
{
    if(pool == NULL)
        return 0;
    pool->shutdown = true;
    pthread_join(pool->adjust,NULL);
    for(int i =0;i < pool->live_thr_num;i++){
        pthread_cond_broadcast(&(pool->queue_not_empty));//通知所有线程，可以退出了
    }
    for(int i = 0;i < pool->live_thr_num;i++){
        pthread_join(pool->pthreads[i],NULL);
    }
    threadpool_free(pool);
    return 0;
}






/*回收线程池占用的内存，销毁锁、条件变量*/
int threadpool_free(threadpool_t* pool)
{
    if(pool == NULL)
        return -1;
    if(pool->task_queue)
        free(pool->task_queue);
    if(pool->pthreads)
        free(pool->pthreads);
    pthread_mutex_lock(&(pool->lock));
    pthread_mutex_destroy(&(pool->lock));
    pthread_mutex_lock(&(pool->thread_counter));
    pthread_mutex_destroy(&(pool->thread_counter));
    pthread_cond_destroy(&(pool->queue_not_empty));
    pthread_cond_destroy(&(pool->queue_not_full));
    free(pool);
    pool = NULL;
    return 0;
}






int get_all_threadnum(threadpool_t *pool)
{
    int all_threadnum = -1;
    pthread_mutex_lock(&(pool->lock));
    all_threadnum = pool->live_thr_num;
    pthread_mutex_unlock(&(pool->lock));
    return all_threadnum;
}

int get_busy_threadnum(threadpool_t *pool)
{
    int busy_threadnum = -1;
    pthread_mutex_lock(&(pool->thread_counter));
    busy_threadnum = pool->busy_thr_num;
    pthread_mutex_unlock(&(pool->thread_counter));
    return busy_threadnum;
}

int is_thread_alive(pthread_t tid)
{
    int kill_rc = pthread_kill(tid, 0);     //发0号信号，测试线程是否存活
    if (kill_rc == ESRCH) {
        return false;
    }
    return true;
}






/*测试*/

#if 1
void* process(void* arg)
{
    printf("pthread %ld waking on task %d\n",pthread_self(),*(int*)arg);
    sleep(1);
    printf("task %d is end\n",*(int*)arg);
    return NULL;
}

int main()
{
    threadpool_t* pool;
    pool = threadpool_creat(3,100,100);
    printf("pool ininted\n");
    int num[80],i;
    for(i = 0;i < 80;i++)
    {
        num[i] = i;
        printf("add task %d\n",i);
        thread_add(pool,process,(void*)&num[i]);
    }
    sleep(10);
    threadpool_destroy(pool);
    return 0;
}


#endif


























