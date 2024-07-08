/**
 * @file scheduler.cc
 * @author 调度器实现
 * @brief
 * @version 0.1
 * @date 2024-07-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "hook.hpp"
#include "scheduler.hpp"

#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

namespace qc {
/// @brief 当前线程的调度器,同一个调度下的所有线程指同一个调度器实例
static thread_local Scheduler *t_scheduler = nullptr;
/// @brief 每个线程独有的 线程调度协程,Main函数也有.
static thread_local Fiber *t_scheduler_fiber = nullptr;

/**
 * @details 协程分为三类:Main线程主协程,线程调度协程,任务协程.
 * 每个线程都有线程调度协程由t_scheduler_fiber记录,Main线程根协程由t_thread_fiber记录,当前运行的协程(不论是线程调度协程,Main线程根协程,任务协程)由t_fiber记录
 *
 */
Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name) {
    qc_assert(threads >= 1);

    _use_caller = use_caller;
    _name = name;

    if (use_caller) {
        threads--;
        Fiber::GetThis();  // 获得协程
        qc_assert(GetThis() == nullptr);
        t_scheduler = this;

        _rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, false));

        t_scheduler_fiber = _rootFiber.get();

        _rootThread = syscall(SYS_gettid);

        _threadIds.push_back(_rootThread);

    } else
        _rootThread = -1;

    t_scheduler = this;

    _threads_count = threads;
}

Scheduler::~Scheduler() {
    qc_assert(_stopping);
    if (GetThis() == this) t_scheduler = nullptr;
}

Fiber* Scheduler::GetMainFiber() {
    return t_scheduler_fiber;
}

Scheduler *Scheduler::GetThis() { return t_scheduler; }

void Scheduler::setThis() { t_scheduler = this; }

void Scheduler::start() {
    MutexType::Lock lock(_mutex);
    if (_stopping) {
        fprintf(stderr, "Schedule is stopped\n");
        return;
    }

    qc_assert(_threads.empty());
    _threads.resize(_threads_count);

    for (size_t i = 0; i < _threads_count; ++i) {
        _threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                                     _name + " " + std::to_string(i)));
        _threadIds.push_back(_threads[i]->getId());
    }
}

void Scheduler::run() {
    std::cout << "begin run" << std::endl;
    set_hook_enable(true);
    setThis();
    // 当前线程不是Main线程
    if (syscall(SYS_gettid) != _rootThread) {
        // 初始化当前线程的第一个协程主协程(调度协程)
        t_scheduler_fiber = Fiber::GetThis().get();
    }

    // 创建idle协程
    Fiber::ptr idleFiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr taskFiber;

    ScheduleTask task;

    while (1) {
        task.reset();
        /// 是否通知其他线程进行任务调度
        bool tickle_me = false;
        {
            MutexType::Lock lock(_mutex);

            // std::cout << "_queue.size() = " << _queue.size() << std::endl;

            auto it = _queue.begin();
            while (it != _queue.end()) {
                // 有调度任务
                // std::cout << "_queue has task" << std::endl;

                if (it->thread != -1 && it->thread != syscall(SYS_gettid)) {
                    tickle_me = true;
                    ++it;
                    continue;
                }
                qc_assert(it->fiber || it->cb);
                if (it->fiber) qc_assert(it->fiber->getState() == Fiber::READY);

                task = *it;
                _queue.erase(it++);
                ++_activeThreadCount;
                break;
            }
            std::cout << "get a task" << std::endl;
            // 当前线程拿到一个线程,任务队列不为空,告诉其他线程
            tickle_me |= (it != _queue.end());
        }
        if (tickle_me) tickle();
        if (task.fiber) {
            task.fiber->resume();
            --_activeThreadCount;
            task.reset();
        } else if (task.cb) {
            // std::cout << "task.cb is not nullptr" << std::endl;
            if (taskFiber) {
                // std::cout << "cur taskFiber is not nullptr" << std::endl;
                taskFiber->reset(task.cb);
            } else {
                // std::cout << "cur taskFiebr is nullptr" << std::endl;
                taskFiber.reset(new Fiber(task.cb));
            }
            // std::cout << "get taskFiber" << std::endl;
            task.reset();
            taskFiber->resume();
            --_activeThreadCount;
            taskFiber.reset();
        } else {
            // 任务队列为空
            if (idleFiber->getState() == Fiber::TERM) {
                std::cout << "idle fiber term" << std::endl;
                break;
            }
            ++_idleThreadCount;
            idleFiber->resume();
            --_idleThreadCount;
        }
    }
    std::cout << "run exit" << std::endl;
}
/// @brief 通知其他线程由epoll实现这里tickle为virtual 后面再实现
void Scheduler::tickle() { std::cout << "tickle" << std::endl; }

bool Scheduler::stopping() {
    MutexType::Lock lock(_mutex);
    return _stopping && _queue.empty() && _activeThreadCount == 0;
}

void Scheduler::idle() {
    while (!stopping()) {
        Fiber::GetThis()->yield();
    }
}

void Scheduler::stop() {
    if (stopping()) return;
    _stopping = true;

    // std::cout << "stopping" << std::endl;

    // std::cout << "_queue.size() = " << _queue.size() << std::endl;

    // stop指令只能由Main线程发起
    // 这里只有一个调度器实例所以,如果使用caller线程GetThis() == this
    if (_use_caller) qc_assert(GetThis() == this);
    else qc_assert(GetThis() != this);

    for (size_t i = 0; i < _threads_count; ++i) {
        tickle();
    }

    if (_rootFiber) {
        _rootFiber->resume();
        std::cout << "root fiber end" << std::endl;
    }

    std::vector<Thread::ptr> threads;
    {
        MutexType::Lock lcok(_mutex);
        threads.swap(_threads);
    }
    for (auto &i : threads) i->join();
}

}  // namespace qc