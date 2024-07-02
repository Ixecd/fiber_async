/**
 * @file thread.cc
 * @author qc
 * @brief 线程封装实现
 * @version 0.1
 * @date 2024-07-02
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <unistd.h>
#include <sys/syscall.h>

#include "thread.hpp"


namespace qc {

static thread_local Thread *t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOWN";

/**
 * @details 由于Thread类的底层还是使用pthread_create创建,所以线程执行函数的类型必须是void* cb(void *),所以这个函数必须是静态类型不能为成员函数,成员函数第一个参数默认是this,所以要将this作为参数传递给cb就可以.
 */
void* Thread::ThreadFunc(void* args) {
    Thread* thr = (Thread*) args;
    t_thread = thr;
    t_thread_name = thr->m_name;
    thr->m_pid = syscall(SYS_gettid);
    // 给线程命名
    pthread_setname_np(pthread_self(), thr->m_name.substr(0, 15).c_str());
    std::function<void()> cb;
    cb.swap(thr->m_cb);
    
    std::cout << "begin callback" << std::endl;
    cb();
    return nullptr;
}

Thread::Thread(std::function<void()> cb, const std::string& name = "UNKNOWN") : m_cb(cb), m_name(name) {
    // 创建线程
    int rt = pthread_create(&m_tid, nullptr, ThreadFunc, this);
    if (rt) {
        std::cout << "pthread_create error, name : " << m_name << std::endl;
        throw std::logic_error("pthread_create");
    }
}

Thread::~Thread() {
    if (m_pid) {
        pthread_detach(m_pid);
    }
}

void Thread::join() {
    if (m_pid) {
        int rt = pthread_join(m_pid, nullptr);
        if (rt) {
            std::cout << "pthread_join error, name : " << m_name << std::endl;
            throw std::logic_error("pthread_join");
        }
        m_pid = 0;
    }
}

Thread* GetThis() {
    return t_thread;
}

const std::string &Thread::GetName() {
    return t_thread_name;
}

void Thread::SetName(const std::string &name) {
    t_thread_name = name;
}

}