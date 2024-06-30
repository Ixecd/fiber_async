/**
 * @file mutex.hpp
 * @author qc
 * @brief 锁的基本封装
 * @version 0.1
 * @date 2024-06-30
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include <pthread.h>
#include <semaphore.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <mutex>

#include "noncopyable.hpp"

namespace qc {

/// @brief 信号量
class Semaphore : public Noncopyable {
public:
    Semaphore(size_t count = 0) {
        if (sem_init(&m_semaphore, 0, count)) 
            throw std::logic_error("sem_init error");
    }

    ~Semaphore() {
        sem_destroy(&m_semaphore);
    }
    void P() {
        if (sem_wait(&m_semaphore)) {
            throw std::logic_error("sem_wait error");
        }
    }
    void V() {
        if (sem_post(&m_semaphore)) {
            throw std::logic_error("sem_post error");
        }
    }

private:
    sem_t m_semaphore;
};

/// @brief 局部锁
/// @details
/// m_locked指的是自己的状态,能不能拿到锁还不好说,有可能阻塞,拿到之后才会重新设置自己的状态
template <class T>
class ScopedLockImpl {
public:
    ScopedLockImpl(T& mutex) : m_mutex(mutex) {
        m_mutex.lock();
        m_locked = true;
    }
    ~ScopedLockImpl() {
        m_mutex.unlock();
        m_locked = false;
    }
    void lock() {
        if (m_locked == false) {
            m_mutex.lock();
            m_locked = true;
        }
    }
    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;
};

/// @brief 局部读锁模板实现
/// @details C++中大多数锁都是不能拷贝的
template <class T>
class ReadScopedLockImpl : public Noncopyable {
public:
    ReadScopedLockImpl(T& mutex) : m_mutex(mutex) {
        m_mutex.rdlock();
        m_locked = true;
    }
    ~ReadScopedLockImpl() { unlock(); }
    void lock() {
        if (!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }
    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;
};

template <class T>
class WriteScopedLockImpl {
public:
    WriteScopedLockImpl(T& mutex) : m_mutex(mutex) {
        m_mutex.wrlock();
        m_locked = true;
    }
    ~WriteScopedLockImpl() { unlock(); }
    void lock() {
        if (!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }
    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;
};

/// @brief 互斥量封装
class Mutex : public Noncopyable {
public:
    /// @brief 每个互斥量里声明一个范围锁
    typedef ScopedLockImpl<Mutex> Lock;

    Mutex() { pthread_mutex_init(&m_mutex, nullptr); }
    ~Mutex() { pthread_mutex_destroy(&m_mutex); }

    void lock() { pthread_mutex_lock(&m_mutex); }

    void unlock() { pthread_mutex_unlock(&m_mutex); }

private:
    /// mutex
    pthread_mutex_t m_mutex;
};

/// @brief 空锁
class NullMutex : public Noncopyable {
public:
    typedef ScopedLockImpl<NullMutex> Lock;

    NullMutex() {}
    ~NullMutex() {}
    void lock() {}
    void unlock() {}
};

class RWMutex : public Noncopyable {
public:
    /// @brief 局部读锁
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    /// @brief 局部写锁
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    RWMutex() { pthread_rwlock_init(&m_lock, nullptr); }
    ~RWMutex() { pthread_rwlock_destroy(&m_lock); }
    void rdlock() { pthread_rwlock_rdlock(&m_lock); }
    void wrlock() { pthread_rwlock_wrlock(&m_lock); }
    void unlock() { pthread_rwlock_unlock(&m_lock); }

private:
    /// @brief 读写锁
    pthread_rwlock_t m_lock;
};

/// @brief 空读写锁
class NullRWMutex : public Noncopyable {
public:
    typedef ReadScopedLockImpl<NullRWMutex> ReadLock;
    typedef WriteScopedLockImpl<NullRWMutex> WriteLock;
    NullRWMutex() {}
    ~NullRWMutex() {}
    void rdlock() {}
    void wrlock() {}
    void unlock() {}
};


/// @brief 自旋锁(CPU忙等)
/// @details 适合临界区比较小的情况
class Spinlock : public Noncopyable {
public:
    /// @brief 局部所
    typedef ScopedLockImpl<Spinlock> Lock;

    Spinlock() {
        pthread_spin_init(&m_mutex, 0);
    }
    ~Spinlock() {
        pthread_spin_destroy(&m_mutex);
    }
    void lock() {
        pthread_spin_lock(&m_mutex);
    }
    void unlock() {
        pthread_spin_unlock(&m_mutex);
    }

private:
    /// @brief 自旋锁
    pthread_spinlock_t m_mutex;
};

/// @brief 原子锁
class CASLock : public Noncopyable {
public:
    /// @brief 局部锁
    typedef ScopedLockImpl<CASLock> Lock;

    CASLock() {
        m_mutex.clear();
    }
    ~CASLock() {}
    void lock() {
        while (std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acq_rel));
    }

    void unlock() {
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }

private:
    /// @brief 原子状态
    volatile std::atomic_flag m_mutex;
};

}  // namespace qc