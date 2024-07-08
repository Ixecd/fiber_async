/**
 * @file scheduler.hpp
 * @author 协程调度简单实现
 * @brief
 * @version 0.1
 * @date 2024-07-01
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once
#include <list>

#include "qc.hpp"
#include "fiber.hpp"
#include "mutex.hpp"
#include "thread.hpp"
namespace qc {

class ScheduleTask {
    friend class Scheduler;
public:
    ScheduleTask() { thread = -1; }

    ScheduleTask(Fiber::ptr f, int thr) {
        fiber = f;
        thread = thr;
    }

    ScheduleTask(Fiber::ptr *f, int thr) {
        fiber.swap(*f);
        thread = thr;
    }

    ScheduleTask(std::function<void()> f, int thr) {
        cb = f;
        thread = thr;
    }

    void reset() {
        fiber = nullptr;
        thread = -1;
        cb = nullptr;
    }

private:
    Fiber::ptr fiber;
    std::function<void()> cb;
    int thread;
};

class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

public:
    Scheduler(size_t threads = 1, bool use_caller = true,
              const std::string &name = "Scheduler");

    virtual ~Scheduler();

    const std::string &getName() const { return _name; }

public:
    /// @brief 启动调度器
    void start();
    /// @brief 停止调度器
    void stop();
    /**
     * @brief 添加调度任务
     *
     * @tparam Fiber_Cb 任务类可以是协程对象或函数指针
     * @param task 任务
     * @param thread 指定的线程执行,-1表示任意线程
     */
    template <class Fiber_Cb>
    void add_task(Fiber_Cb task, int thread = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(_mutex);
            need_tickle = add_task_nolock(task, thread);
        }
        if (need_tickle) tickle();
    }

    template<class Fiber_Cb>
    bool add_task_nolock(Fiber_Cb t, int thread) {
        bool need_tickle = false;
        if (_queue.empty()) need_tickle = true;
        ScheduleTask task(t, thread);
        _queue.push_back(task);
        std::cout << "add task sucess" << std::endl;
        return need_tickle;
    }

    /// @brief 协程调度函数
    void run();

protected:
    /// @brief 空闲协程
    virtual void idle();
    /// @brief 通知调度器由任务
    virtual void tickle();
    /// @brief 返回是否可以停止
    virtual bool stopping();
    /// @brief 设置当前协程调度器
    void setThis();
    /// @brief 当前是否有空闲协程
    bool hasIdleThreads() { return _idleThreadCount > 0; }

public:
    static Fiber* GetMainFiber();

    static Scheduler *GetThis();

private:
    /// @brief 调度器名称
    std::string _name;
    /// @brief 任务队列
    std::list<ScheduleTask> _queue;
    /// @brief 互斥锁
    MutexType _mutex;
    /// @brief 线程池
    std::vector<Thread::ptr> _threads;
    /// @brief 线程ID数组
    std::vector<pid_t> _threadIds;
    /// @brief 工作线程数量,不包括use_caller的主线程
    size_t _threads_count;
    /// @brief 活跃线程数量
    std::atomic<size_t> _activeThreadCount{0};
    /// @brief 空闲线程数量
    std::atomic<size_t> _idleThreadCount{0};
    /// @brief 是否使用use caller
    bool _use_caller;
    /// @brief 使用use_caller时的Main线程主协程(根协程)
    Fiber::ptr _rootFiber;
    /// @brief 使用use_caller时调度器所在线程的id
    long int _rootThread = 0;
    /// @brief 是否正在停止
    bool _stopping = false;
};

}  // namespace qc