/**
 * @file timer.hpp
 * @author qc
 * @brief 定时器的封装
 * @details 定时器只能由TimerManager创建
 * @version 0.1
 * @date 2024-07-05
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include <ctime>
#include <functional>
#include <memory>
#include <set>
#include <vector>

#include "mutex.hpp"
#include "noncopyable.hpp"
namespace qc {

class TimerManager;

uint64_t GetElapsedMS();

class Timer : public std::enable_shared_from_this<Timer> {
    friend class TimerManager;
    friend class IOManager;
public:
    typedef std::shared_ptr<Timer> ptr;

    bool cancel();

    bool reset(uint64_t ms, bool from_now);

    bool refresh();

private:
    Timer(uint64_t ms, std::function<void()> cb, bool recurring,
          TimerManager* manager);

    // 专门用来比较的
    Timer(uint64_t next);

private:
    /// @details 仿函数,定时器比较函数.
    class Comparator {
    public:
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };

private:
    /// @brief 执行周期
    uint64_t m_ms = 0;
    /// @brief 精确的执行时间
    uint64_t m_next;
    /// @brief 定时器对应的回调函数
    std::function<void()> m_cb;
    /// @brief 是否循环
    bool m_recurring;
    /// @brief 管理器
    TimerManager* m_manager = nullptr;
};

/// @brief
/// 一般在类中定义了虚析构函数,同时还要定义拷贝构造函数,拷贝赋值函数,移动构造,移动赋值
class TimerManager : public std::enable_shared_from_this<TimerManager> {
    friend class Timer;
    friend class IOManager;
public:
    typedef std::shared_ptr<TimerManager> ptr;
    typedef RWMutex RWMutexType;

    TimerManager();
    virtual ~TimerManager();

    Timer::ptr add_timer(uint64_t ms, std::function<void()> cb, bool recurring = false);

    uint64_t getNextTimer();

    void listExpiredCb(std::vector<std::function<void()>>& cbs);

    bool hasTimer() {
        RWMutexType::ReadLock lock(m_mutex);
        return m_timers.size();
    }

protected:
    virtual void OnTimerInsertedAtFront() = 0;

    void add_timer(Timer::ptr timer, RWMutex::WriteLock& lock);

    bool detectClockRollover(uint64_t now_ms);

private:
    /// @brief 小根堆放定时器
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    /// @brief 读写锁
    RWMutexType m_mutex;
    /// @brief 是否触发onTimerInsertedAtFront
    bool m_tickled = false;
    /// @brief 上次执行时间
    uint64_t m_previousTime = 0;
};

}  // namespace qc