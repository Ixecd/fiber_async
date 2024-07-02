/**
 * @file thread.hpp
 * @author qc
 * @brief 封装线程
 * @version 0.1
 * @date 2024-07-02
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <iostream>
#include <pthread.h>
#include <memory>
#include <functional>
#include <string>

namespace qc {

/**
 * @brief 封装线程类
 * 
 */
class Thread {
public:
    typedef std::shared_ptr<Thread> ptr;

    Thread(std::function<void()> cb, const std::string& name);

    ~Thread();

    pid_t getId() const { return m_pid; }

    void join();

public:
    /// @brief 线程执行函数
    static void* ThreadFunc(void* args);

    static Thread* GetThis();

    static const std::string &GetName();

    static void SetName(const std::string &name);

private:
    Thread(const Thread&) = delete;

    Thread(const Thread&&) = delete;

    Thread operator=(const Thread&) = delete;

private:
    /// @brief 线程号
    pid_t m_pid;
    /// @brief 线程标识符
    pthread_t m_tid;
    /// @brief 回调函数
    std::function<void()> m_cb;
    /// @brief 名称
    std::string m_name;
    

};


}