/**
 * @file timer.cc
 * @author qc
 * @brief 定时器实现
 * @version 0.1
 * @date 2024-07-05
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "timer.hpp"

#include <sys/time.h>
#include <ctime>

#include "mutex.hpp"
#include "qc.hpp"
namespace qc {

uint64_t GetElapsedMS() {
    struct timespec ts {
        0
    };
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

bool Timer::Comparator::operator()(const Timer::ptr& lhs,
                                   const Timer::ptr& rhs) const {
    if (!lhs && !rhs) return false;
    if (!lhs) return true;
    if (!rhs) return false;
    if (lhs->m_next < rhs->m_next) return true;
    if (rhs->m_next < lhs->m_next) return false;
    return lhs.get() < rhs.get();
}

Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring,
             TimerManager* manager)
    : m_ms(ms), m_cb(cb), m_recurring(recurring), m_manager(manager) {
    m_next = m_ms + GetElapsedMS();
}

Timer::Timer(uint64_t next) : m_next(next) {}

/// @brief 作为一个定时器,自己可以通过TimerManager取消自己
bool Timer::cancel() {
    RWMutex::WriteLock lock(m_manager->m_mutex);
    if (m_cb) {
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

bool Timer::refresh() {
    RWMutex::WriteLock lock(m_manager->m_mutex);
    if (!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    m_next = GetElapsedMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}

bool Timer::reset(uint64_t ms, bool from_now) {
    if (ms == m_ms && !from_now) return true;
    RWMutex::WriteLock lock(m_manager->m_mutex);
    if (!m_cb) return true;
    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end()) return false;
    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if (from_now) {
        start = GetElapsedMS();
    } else {
        start = m_next - m_ms;
    }
    m_ms = ms;
    m_next = start + m_ms;
    m_manager->add_timer(shared_from_this(), lock);
    return true;
}

TimerManager::TimerManager() { m_previousTime = GetElapsedMS(); }

TimerManager::~TimerManager() {}

Timer::ptr TimerManager::add_timer(uint64_t ms, std::function<void()> cb,
                                   bool recurring) {
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    add_timer(timer, lock);
    return timer;
}

void TimerManager::add_timer(Timer::ptr timer, RWMutexType::WriteLock& lock) {
    // it 指向插入的数据
    auto it = m_timers.insert(timer).first;
    bool tickle = (it == m_timers.begin() && !m_tickled);
    if (tickle) m_tickled = true;
    lock.unlock();
    if (tickle) OnTimerInsertedAtFront();
}

bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollover = false;
    if (now_ms < m_previousTime && now_ms < (m_previousTime - 60 * 60 * 1000))
        rollover = true;
    m_previousTime = now_ms;
    return rollover;
}

void TimerManager::listExpiredCb(std::vector<std::function<void()>>& cbs) {
    // std::cout << "listExpiredCb.." << std::endl;
    uint64_t now_ms = GetElapsedMS();
    std::vector<Timer::ptr> expired;
    // std::cout << "in listExpiredCb m_timer.size() = " << m_timers.size() <<
    // std::endl;
    {
        RWMutexType::ReadLock lock(m_mutex);
        if (m_timers.empty()) return;
    }
    RWMutexType::WriteLock lock(m_mutex);
    if (m_timers.empty()) return;

    bool rollover = false;
    if (detectClockRollover(now_ms)) rollover = true;
    if (!rollover && ((*m_timers.begin())->m_next > now_ms)) return;

    Timer::ptr now_timer(new Timer(now_ms));
    auto it = rollover ? m_timers.end() : m_timers.upper_bound(now_timer);
    // auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
    // while (it != m_timers.end() && (*it)->m_next <= now_ms) ++it;

    expired.insert(expired.begin(), m_timers.begin(), it);
    // std::cout << "after insert expired.size() = " << expired.size() <<
    // std::endl;
    m_timers.erase(m_timers.begin(), it);

    cbs.reserve(expired.size());
    for (auto& timer : expired) {
        cbs.push_back(timer->m_cb);
        if (timer->m_recurring) {
            timer->m_next = GetElapsedMS() + timer->m_ms;
            m_timers.insert(timer);
        } else
            timer->m_cb = nullptr;
    }
}

uint64_t TimerManager::getNextTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    if (m_timers.empty()) return ~0ull;
    // std::cout << "cur m_timers.size() = " << m_timers.size() << std::endl;
    const Timer::ptr& next = *m_timers.begin();
    uint64_t now_ms = GetElapsedMS();
    if (now_ms >= next->m_next)
        return 0;
    else
        return next->m_next - now_ms;
}

}  // namespace qc