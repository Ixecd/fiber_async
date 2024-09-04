# 基于ucontext_t(GUN C)和C++11实现多线程协程IO调度器
**核心四件套**

**getcontext**

**setcontext**

**makecontext**

**swapcontext**

- 设计思路如下:

    1.协程(Fiber) : 分为`Main线程主协程`、`线程主协程`、`任务协程`,其中Main线程主协程又称为根协程,有栈(为的是让主线程也参与进来调度,其实是为了收尾).线程主协程负责调度任务协程,无栈.任务协程负责处理任务,有栈.线程主协程的数量是线程池中线程的总数,Main线程主协程只有一个,任务协程有多个.
    
    2.调度器(Scheduler->Loop) : 负责调度协程,调度器在主线程创建,可以指定创建线程的数量,以及是否让主线程也参与进来调度,是的话就创建Main线程主协程,负责不创建.调度器负责创建线程池,
      绑定process不断判断任务队列中是否有任务,有的话就执行.
    
    3.协程之间的切换 : 多线程下每个线程有一个线程主协程(`Master`),负责调度.这个线程下会生成一些任务协程(`Slaves`), 使用`static thread_local` 每个线程各自记录自己的Master, 任务协程执行完(其必然参与调度器调度),与Master进行swap,切换到Master执行,之后由Master重新在任务队列中选择一个Slave执行.而Master作为调度者,其不参与调度,所以如果调度器被停止,Master会和Main线程主协程切换(Main线程主协程也不参与调度,负责最后收尾),之后由Mian线程主协程执行,
      由其再次将所有队列中可能剩余的任务协程都执行一遍,再真正关闭整个调度器.
    
    4.定时器 : 基于`set`实现,增删改查的效率都为O(logN),定时器中设置自己的Comparation,将剩余时间最小的放到最前面,这样可以实现类似于`when_any`的效果.定时器中包括任务协程以及触发事件点.
    
    5.IO调度 : 基于`epoll`实现,继承Scheduler和TimerManager,由其创建epoll,增删改查EpollEvent,override idle, 由线程主协程(Master)来执行`idle`,不断判断是否有事件到达、是否有定时器到达.
      epoll_wait中的TIMEOUT设置为定时器中最小的那个和默认5s的最小值.触发的模式为ET(fd状态改变才会触发),事件触发一次删除一次,回调函数由任务协程负责.
    
    6.Hook : 使用外挂式Hook, extern "C" { ... }; 实现异步.eg:两个任务一个要sleep 1s, 一个要sleep 2s,同步下一共需要sleep 3s, 异步下只需要sleep 2s. 这里的Hook就是为了在sleep中通过添加定时器,
      fd操作中等待IO事件达到异步的效果.
    
    7.为了避免内存泄漏,采用RAII思想,使用智能指针封装.多线程下为保障数据安全,使用封装好的符合RAII思想的mutex实现(`std::unique_lock`也行),同时使用`static thread_local`, `std::atomic<int>`来保证数据之间的独立.考虑到使用互斥锁会导致性能上的损耗,在临界区相对小的地方使用自旋锁,在很小的地方直接使用`std::atomic`来原子保证安全.
    
    8.单例模式 : 使用局部静态变量实现懒汉式单例模式.
