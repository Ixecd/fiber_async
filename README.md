# Coroutine Library
1. 关于Fiber类
    协程分为Main线程主协程、线程主协程、一般协程,其中Main线程主协程和线程主协程负责调度,没有cb也没有stack,使用一个bool变量m_runInScheduler来判断当前协程是否run在调度其上,如果这个值为false表示当前协程是线程主协程,swapcontext需要和Main线程主协程进行交换.
    
2. 关于namespace std
    切记一般不要在头文件中使用using namespce std 因为可能导致歧义

3. 关于消息队列
    消息队列中底层数据结构使用队列还是使用双向链表?考虑到任务可以被指定特定的线程上运行,所以有可能不满足队列先进先出的特性,队列中不提供迭代器,所以底层使用双向链表较好.

4. 关于回调函数
    以后一般优先使用std::function<void()> 类型的函数作为回调函数,使用std::bind(func, ...);作为其参数,相当于回调函数的参数可以有任意多个.
