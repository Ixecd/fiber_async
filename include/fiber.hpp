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

#include <iostream>
#include <memory>

// 默认一个协程栈的大小为128KB
#define DEFAULT_STACKSIZE 128 * 1024

class Fiber : public std::enable_shared_from_this<Fiber> {
public:
public:
    enum STATE { READY = 0, RUNNING = 1, TERM = 2 };

private:
    /// @brief 当前协程上下文
    ucontext_t m_ctx;
    /// @brief 当前协程状态
    STATE m_state;
    /// @brief 当前栈大小
    unsigned long long m_stack_size;
    /// @brief 栈指针
    char *m_stack;
};