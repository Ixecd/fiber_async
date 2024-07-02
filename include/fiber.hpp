/**
 * @file fiber.cc
 * @author qc
 * @brief 最基本协程类的封装
 * @version 0.1
 * @date 2024-06-30
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include <ucontext.h>
#include <functional>
#include <iostream>
#include <memory>

#include "qc.hpp"

// 默认一个协程栈的大小为128KB
#define DEFAULT_STACKSIZE 128 * 1024

namespace qc {

class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    typedef std::shared_ptr<Fiber> ptr;
public:
    enum STATE { READY = 0, RUNNING = 1, TERM = 2 };
    ~Fiber();
private:
    /// @brief 线程主协程才会调用这个函数
    Fiber();
public:
    /// @brief 一般协程
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool run_in_scheduler = true);

    void reset(std::function<void()> cb);

    void resume();

    void yield();

    uint64_t git_id() const { return m_id; }

    STATE getState() const { return m_state; }

public:
    static void SetThis(Fiber* f);

    static ptr GetThis();

    static uint64_t TotalFibers();
    /// @brief 协程入口函数
    static void MainFunc();

    static uint64_t GetFiberId(); 

private:
    /// @brief 协程id
    uint64_t m_id           = 0;
    /// @brief 当前协程上下文
    ucontext_t m_ctx;
    /// @brief 当前协程状态
    STATE m_state           = READY;
    /// @brief 当前栈大小
    size_t m_stacksize      = 0;
    /// @brief 栈指针
    void *m_stack           = nullptr;
    /// @brief 回调函数,这里只支持无参且返回类型为void的,之后可以使用bind绑定各种参数
    std::function<void()> m_cb;
    /// @brief 是否参与调度器调度
    bool m_runInScheduler;
};

}  // namespace qc