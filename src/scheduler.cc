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
#include "scheduler.hpp"

namespace qc {
/// @brief 当前线程的调度器,同一个调度下的所有线程指同一个调度器实例
static thread_local Scheduler *t_scheduler = nullptr;
/// @brief 每个线程独有的 线程调度协程,Main函数也有.
static thread_local Fiber *t_scheduler_fiber = nullptr;

/**
 * @details 协程分为三类:Main线程主协程,线程调度协程,任务协程. 每个线程都有线程调度协程由t_scheduler_fiber记录,Main线程根协程由t_thread_fiber记录,当前运行的协程(不论是线程调度协程,Main线程根协程,任务协程)由t_fiber记录
 * 
 */
Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name) {
    qc_assert(threads > 1);

    _use_caller = use_caller;
    _name = name;

    if (use_caller) {
        threads--;
        Fiber::GetThis(); //获得根协程
        qc_assert(GetThis() == nullptr);
        t_scheduler = this;

        _rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, false));

        t_scheduler_fiber = _rootFiber.get();

        _rootThread = pthread_self();

        _threadIds.push_back(_rootThread);

    } else _rootThread = -1;

    _threads_count = threads;
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

void Scheduler::start() {
    MutexType::Lock lock(_mutex);
    if (_stopping) {
        fprintf(stderr, "Schedule is stopped\n");
        return;
    }

    qc_assert(_threadIds.size() == 0);

    _threadIds.resize(_threads_count);
    pthread_t tid;
    int rt;
    for (size_t i = 0; i < _threads_count; ++i) {
        rt = pthread_create(&tid, nullptr, run, nullptr);
        _threadIds.push_back(tid);
    }
}

void* Scheduler::run(void *args) {

}

}