/**
 * @file iomanager.hpp
 * @author qc
 * @brief 封装IOManager
 * @version 0.1
 * @date 2024-07-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once
#include "scheduler.hpp"
#include "timer.hpp"

namespace qc {

enum Event {
        NONE = 0x0,
        READ = 0x1,
        WRITE = 0x4
};

class FdContext {
    friend class IOManager;
public:
    typedef Mutex MutexType;
    struct EventContext {
        Scheduler *scheduler = nullptr;
        Fiber::ptr fiber = nullptr;
        std::function<void()> cb = nullptr;
    };
    // 获取事件上下文
    EventContext &getEventContext(Event event);

    void resetEventContext(EventContext &ctx);

    void triggerEvent(Event event);

private:
    int m_fd;
    Event m_events = NONE;
    EventContext m_read;
    EventContext m_write;
    /// @brief 事件的锁 共享资源是Event
    MutexType m_mutex;
};

class IOManager : public Scheduler, public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;
    
    IOManager(size_t threads = 1, bool use_caller = true , const std::string &name = "IOManager");

    ~IOManager();

    void contextResize(size_t size) {
        m_fdContexts.resize(size);
        for (size_t i = 0; i < size; ++i) {
            if (!m_fdContexts[i]) {
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->m_fd = i;
            }
        }
    }
public:

    int addEvent(int fd, Event event, std::function<void()> cb);

    bool delEvent(int fd, Event event);

    bool cancelEvent(int fd, Event event);

    bool cancelAll(int fd);

public:
    void tickle() override;

    void idle() override;

    bool stopping() override;

    static IOManager* GetThis();

    void OnTimerInsertedAtFront() override;

private:
    int m_epfd;
    /// @brief 这里使用管道触发事件,Lars中每个消息队列单独设置了一个eventfd来触发事件,每个消息队列中都有一个epoll类
    int m_tickleFds[2];

    std::atomic<size_t> m_pendingEventCount {0};

    std::vector<FdContext*> m_fdContexts;

    RWMutexType m_mutex;
};


}