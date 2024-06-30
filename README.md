# Coroutine Library
1. 关于Fiber类
    协程分为Main线程主协程、线程主协程、一般协程,其中Main线程主协程和线程主协程负责调度,没有cb也没有stack,使用一个bool变量m_runInScheduler来判断当前协程是否run在调度其上,如果这个值为false表示当前协程是线程主协程,swapcontext需要和Main线程主协程进行交换.
    
2. 关于namespace std
    切记一般不要在头文件中使用using namespce std; 因为可能导致歧义