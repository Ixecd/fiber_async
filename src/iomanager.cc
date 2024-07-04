/**
 * @file iomanager.cc
 * @author qc
 * @brief IOManager封装
 * @version 0.1
 * @date 2024-07-03
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "iomanager.hpp"

#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <cstring>

namespace qc {

FdContext::EventContext &FdContext::getEventContext(Event event) {
    switch(event) {
        case READ:
            return m_read;
        case WRITE:
            return m_write;
        default :
            throw std::logic_error("getContext error : unknow event");
    }
}

void FdContext::resetEventContext(FdContext::EventContext &ctx) {
    ctx.cb = nullptr;
    // 智能指针直接reset
    ctx.fiber.reset();
    ctx.scheduler = nullptr;
}

void FdContext::triggerEvent(Event event) {
    std::cout << "triggerEvent event : " << event << std::endl;
    qc_assert(m_events & event);
    // 这里触发完之后不需要去除事件,因为一次触发对应一次删除
    // m_events = (Event)(m_events & ~event);
    EventContext &ctx = getEventContext(event);
    if (ctx.cb) {
        std::cout << "ctx.cb is not nullptr" << std::endl;
        if (ctx.scheduler == nullptr) std::cout << "ctx.scheduler is nullptr" << std::endl;
        else std::cout << "ctx.scheduler is not nullptr" << std::endl;
        ctx.scheduler->add_task(ctx.cb);
    } else ctx.scheduler->add_task(ctx.fiber);
    resetEventContext(ctx);
    std::cout << " add_task in triggerEvent succ " << std::endl;
    return;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
    : Scheduler(threads, use_caller, name) {
    m_epfd = epoll_create(5000);
    // 返回一个文件描述符
    qc_assert(m_epfd > 0);
    // 创建管道(匿名),用于tickle调度协程
    int rt = pipe(m_tickleFds);
    qc_assert(!rt);

    // 下面给管道的两个口注册io事件用来触发tickle事件
    epoll_event event;
    bzero(&event, sizeof(event));
    // 这里读端只有状态改变才会触发,因为tickle没有必要所有的都触发,提高效率,同时文件描述符要设置成非阻塞状态
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    // 成功返回0
    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    qc_assert(!rt);

    // 添加成功返回0
    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    qc_assert(!rt);

    contextResize(32);
    // 调用Scheduler中的start开始创建线程执行调度
    start();
}

void IOManager::tickle() {
    if (!hasIdleThreads()) return;
    // 有空闲线程就往管道中写,触发读事件
    int rt = write(m_tickleFds[1], "1", 1);
    qc_assert(rt == 1);
}

// 下面是核心函数
/**
 * Q: 匿名管道是否有用?
 * A:
 * 每个线程空闲之后都会进入idle协程执行idle线程,在这里设定了超时时间,每次epoll_wait都会返回结
 *    处理完所有事件,也有可能没有事件,执行完就会将自己yield().让调度器去再处理任务
 *    也就是说每进行一次epoll_wait就会触发一次yield(),所以目前这里的tickle只是简单为了提示而已
 */
void IOManager::idle() {
    std::cout << "idle()" << std::endl;
    // 空闲线程执行这个函数,一直监听是否有事件到达
    const uint64_t MAX_EVENTS = 256;
    epoll_event *events = new epoll_event[MAX_EVENTS]{};
    // 下面使用智能指针将其包围起来,并且自定义析构
    std::shared_ptr<epoll_event> shared_events(
        events, [](epoll_event *ptr) { delete[] ptr; });

    while (true) {
        if (stopping()) {
            std::cout << "name = " << getName() << "idle stopping exit"
                      << std::endl;
            break;
        }
        // 下面设定最大的阻塞事件
        static const int MAX_TIMEOUT = 5000;
        // rt == 0 超时
        int rt = epoll_wait(m_epfd, events, MAX_EVENTS, MAX_TIMEOUT);
        if (rt < 0) {
            if (errno == EINTR) continue;
            std::cout << "epoll_wait(" << m_epfd << ") (rt = " << rt
                      << ") (errno = " << errno
                      << " ) (errset : " << strerror(errno) << " )";
            break;  // 直接当前协程结束执行
        }

        for (int i = 0; i < rt; ++i) {
            std::cout << "get event" << std::endl;
            epoll_event &event = events[i];
            if (event.data.fd == m_tickleFds[0]) {
                std::cout << "tickle().." << std::endl;
                uint8_t dummy[256];
                // 由于这里m_tickleFds的触发模式为ET,所以下面要用while一直读完才行
                while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }

            FdContext *fd_ctx = (FdContext *)event.data.ptr;
            // 对事件操作要加锁
            // 这里的问题,这里加了一次锁,后面del的时候还要加锁,加了两次锁
            FdContext::MutexType::Lock lock(fd_ctx->m_mutex);

            /**
             * EPOLLERR: 出错
             * EPOLLHUP: 套接字对端关闭
             * 触发这两种事件,应该同时触发fd的读和写事件,否则可能出现注册的事件永远执行不到的情况.
             */
            if (event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->m_events;
            }
            int real_events = NONE;
            if (event.events & EPOLLIN) real_events |= READ;
            if (event.events & EPOLLOUT) real_events |= WRITE;

            if ((fd_ctx->m_events & real_events) == NONE) continue;

            // 剔除已经发生的事件
            // int left_events = (fd_ctx->m_events & ~real_events);
            // int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

            // // READ == EPOLLIN -> 0x001
            // // WRITE == EPOLLOUT -> 0x004
            // event.events = EPOLLET | left_events;

            // rt = epoll_ctl(m_epfd, op, fd_ctx->m_fd, &event);
            // qc_assert(!rt);

            lock.unlock();

            // 处理已经发生的事件
            if (real_events & READ) {
                fd_ctx->triggerEvent(READ);
                delEvent(fd_ctx->m_fd, READ);
                --m_pendingEventCount;
            }
            if (real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                delEvent(fd_ctx->m_fd, WRITE);
                --m_pendingEventCount;
            }
        }
        // 处理完所有事件
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        raw_ptr->yield();
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext *fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    // 之前就有
    if ((int)m_fdContexts.size() > fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
        fd_ctx->m_fd = fd;
    }

    int op = fd_ctx->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;

    //! 同一个fd不允许重复添加相同的事件
    FdContext::MutexType::Lock lock2(fd_ctx->m_mutex);
    if (fd_ctx->m_events & event) throw std::logic_error("add same event type");

    fd_ctx->m_events = (Event)(fd_ctx->m_events | event);
    FdContext::EventContext &event_ctx = fd_ctx->getEventContext(event);
    qc_assert(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);
    // 赋值schuduler 和回调函数,如果回调函数为空,则把当前协程当成回调执行体
    // ---
    event_ctx.scheduler = Scheduler::GetThis();
    if (cb) {
        event_ctx.cb.swap(cb);
    } else {
        // --===-- ?
        event_ctx.fiber = Fiber::GetThis();
        qc_assert(event_ctx.fiber->getState() == Fiber::RUNNING);
    }
    // ---

    epoll_event epevent;
    epevent.events = fd_ctx->m_events | EPOLLET;
    epevent.data.ptr = fd_ctx;

    // fd 不存在
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    std::cout << "epoll_ctl return : " << rt << " errno = " << strerror(errno) << std::endl;
    qc_assert(!rt);

    ++m_pendingEventCount;

    // 添加完之后,由IOManager::idle()进行触发,触发完之后由其删除

    // 添加完之后要触发
    // fd_ctx->triggerEvent(event);

    // 触发完手动删除
    // delEvent(fd, event);

    return 0;
}

bool IOManager::delEvent(int fd, Event event) {
    std::cout << "in delEvent" << std::endl;
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) return false;
    FdContext *fd_ctx = m_fdContexts[fd];
    lock.unlock();

    std::cout << "before get FdContext::MutexType::Lock" << std::endl;
    // 没拿到锁
    FdContext::MutexType::Lock lock2(fd_ctx->m_mutex);
    std::cout << "after get FdContext::MutexType::Lock" << std::endl;
    if (!(fd_ctx->m_events & event)) {
        std::cout << "del not exits event" << std::endl;
        return false;
    }

    // 清除指定的事件
    Event real_event = (Event)(fd_ctx->m_events & ~event);
    int op = real_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = real_event;
    epevent.data.ptr = fd_ctx;
    // epevent.data.fd = fd_ctx->m_fd;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    qc_assert(!rt);

    --m_pendingEventCount;

    // 清除FdContext中的EventContext
    fd_ctx->m_events = real_event;
    FdContext::EventContext &event_ctx = fd_ctx->getEventContext(event);
    fd_ctx->resetEventContext(event_ctx);

    std::cout << "delEvent succ" << std::endl;
    return true;
}

/// @brief
/// 这里的取消是指不再监听对应文件描述符的事件,但之前向该文件描述符中注册的信息不会改变
///        也就是说并不会对FdContext中的EventContext进行操作,del就需要
bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) return false;
    FdContext *fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->m_mutex);
    if (!(fd_ctx->m_events & event)) return false;

    // 删除前触发一次事件
    fd_ctx->triggerEvent(event);

    // 删除事件
    Event real_event = (Event)(fd_ctx->m_events & ~event);
    int op = real_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = real_event;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    qc_assert(!rt);
    --m_pendingEventCount;

    fd_ctx->m_events = real_event;

    return true;
}

bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) return false;
    FdContext *fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->m_mutex);
    if (!fd_ctx->m_events) return false;

    // 取消之前触发一遍所有的事件
    if (fd_ctx->m_events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if (fd_ctx->m_events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = NONE;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    qc_assert(!rt);

    fd_ctx->m_events = NONE;

    // 之前的逻辑并没有对fd_ctx中的m_events进行操作,为什么这里就会变成NONE??
    qc_assert(fd_ctx->m_events == NONE);
    return true;
}

IOManager::~IOManager() {
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for (size_t i = 0; i < m_fdContexts.size(); ++i) 
        if (m_fdContexts[i]) delete m_fdContexts[i];
}

bool IOManager::stopping() {
    return m_pendingEventCount == 0 && Scheduler::stopping();
}

IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

}  // namespace qc