/**
 * @file fiber.cc
 * @author qc
 * @brief 协程基本类封装
 * @version 0.1
 * @date 2024-06-30
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "fiber.hpp"

#include <atomic>

namespace qc {

/// @brief 线程静态局部变量
static thread_local Fiber *t_fiber = nullptr;
static thread_local Fiber::ptr t_thread_fiber = nullptr;

/// @brief 全局静态变量
static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};

/**
 * @brief malloc栈内存分配器
 */
class MallocStackAllocator {
public:
    static void *Alloc(size_t size) { return malloc(size); }
    static void Dealloc(void *vp, size_t size) { return free(vp); }
};

using StackAllocator = MallocStackAllocator;

void Fiber::SetThis(Fiber *f) { t_fiber = f; }

/// @brief 线程主协程
/// @details 这个协程只能由GetThis()方法获取
Fiber::Fiber() {
    SetThis(this);
    m_state = RUNNING;

    int rt = getcontext(&m_ctx);
    qc_assert(rt == 0);

    ++s_fiber_count;
    m_id = s_fiber_id++;
    // makecontext(&m_ctx, MainFunc, 0);
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool run_in_scheduler)
    : m_id(s_fiber_id++), m_cb(cb) {
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : DEFAULT_STACKSIZE;
    m_stack = StackAllocator::Alloc(m_stacksize);
    int rt = getcontext(&m_ctx);
    qc_assert(rt == 0);

    // getcontext之后要设置m_ctx中的相关属性
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, MainFunc, 0);
}

Fiber::~Fiber() {
    --s_fiber_count;
    if (m_stack) {
        qc_assert(m_state == TERM);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        qc_assert(!m_cb);
        qc_assert(m_state == RUNNING);

        Fiber *cur = t_fiber;
        if (cur == this) SetThis(nullptr);
    }
}

Fiber::ptr Fiber::GetThis() {
    if (t_fiber) return t_fiber->shared_from_this();
    // 下面创建线程的第一个协程
    Fiber::ptr main_fiber(new Fiber);
    qc_assert(t_fiber == main_fiber.get());
    // 这是创建的第一个协程,也就是Main线程主协程
    t_thread_fiber = main_fiber;
    return t_fiber->shared_from_this();
}

uint64_t Fiber::TotalFibers() { return s_fiber_count; }

uint64_t Fiber::GetFiberId() {
    qc_assert(t_fiber != nullptr);
    return t_fiber->m_id;
}

/// @brief 只有任务协程才可以被reset
/// @param cb 
void Fiber::reset(std::function<void()> cb) {
    qc_assert(m_stack);
    // 为了简化状态只允许TERM状态的协程可以被重置
    qc_assert(m_state == TERM);
    m_cb = cb;
    // 这里需不需要重新获取上下文? 需要
    int rt = getcontext(&m_ctx);
    qc_assert(rt == 0);

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, MainFunc, 0);
    m_state = READY;
}

void Fiber::resume() {
    qc_assert(m_state == READY);
    SetThis(this);
    m_state = RUNNING;
    if (m_runInScheduler) {
        /// @brief 跑在调度器上,应该和线程主协程互换
        swapcontext(&(Scheduler::GetMainFiber()->m_ctx) ,&m_ctx);
    } else {
        /// @brief 不跑在调度器上,和Main线程主协程互换
        swapcontext(&(t_thread_fiber->m_ctx), &m_ctx);
    }
}

void Fiber::yield() {
    qc_assert(m_state == RUNNING);
    m_state = READY;
    /// @details 一个协程的yeild()操作必定会回到线程主协程,之后由线程主协程来判断调度下一个协程
    SetThis(t_thread_fiber.get());
    if (m_runInScheduler) {
        swapcontext(&m_ctx, &(Scheduler::GetMainFiber()->m_ctx));
    } else {
        swapcontext(&m_ctx, &(t_thread_fiber->m_ctx)); 
    }
}

/// @brief 只有任务协程才会有这个函数,Main线程主协程和线程主协程都没有回调函数,也就不会调用这个函数
void Fiber::MainFunc() {
    ptr cur = GetThis();
    qc_assert(cur);

    cur->m_cb();
    cur->m_cb = nullptr;
    cur->m_state = TERM;

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->yield();
}

}  // namespace qc