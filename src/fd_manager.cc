/**
 * @file fd_manager.cc
 * @author qc
 * @brief 文件描述符封装类
 * @version 0.1
 * @date 2024-07-06
 *
 * @copyright Copyright (c) 2024
 *
 */


#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include "fd_manager.hpp"
#include "hook.hpp"

namespace qc {

FdCtx::FdCtx(int fd)
    : m_isInit(false),
      m_isSocket(false),
      m_sysNonblock(false),
      m_userNonblock(false),
      m_isClosed(false),
      m_fd(fd),
      m_recvTimeout(-1),
      m_sendTimeout(-1) {
    init();
}

FdCtx::~FdCtx() {}

bool FdCtx::init() {
    if (m_isInit) return true;
    m_recvTimeout = -1;
    m_sendTimeout = -1;

    // 获取文件状态信息
    struct stat fd_stat;
    if (-1 == fstat(m_fd, &fd_stat)) {
        m_isInit = false;
        m_isSocket = false;
    } else {
        m_isInit = true;
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
    }

    if (m_isSocket) {
        int flags = fcntl(m_fd, F_GETFL, 0); // hook 之后的
        if (!(flags & O_NONBLOCK)) fcntl(m_fd, F_SETFL, flags | O_NONBLOCK);
        m_sysNonblock = true;
    } else m_sysNonblock = false;

    m_userNonblock = false;
    m_isClosed = false;
    return m_isInit;
}

void FdCtx::setTimeout(int type, uint64_t v) {
    if (type == SO_RCVTIMEO) 
        m_recvTimeout = v;
    else m_sendTimeout = v;
}

uint64_t FdCtx::getTimeout(int type) {
    return type == SO_RCVTIMEO ? m_recvTimeout : m_sendTimeout;
}

FdManager::FdManager() { m_datas.resize(64); }

FdCtx::ptr FdManager::get(int fd, bool auto_create) {
    if (-1 == fd) return nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_datas.size() <= fd) {
        if (auto_create == false) return nullptr;
    } else {
        if (m_datas[fd] || !auto_create) return m_datas[fd];
    }
    lock.unlock();
    // m_datas里没有auto_create为true

    RWMutexType::WriteLock lock2(m_mutex);
    FdCtx::ptr ctx(new FdCtx(fd));
    if (fd >= (int)m_datas.size()) 
        m_datas.resize(fd * 1.5);
    m_datas[fd] = ctx;
    return ctx;
}

void FdManager::del(int fd) {
    RWMutexType::WriteLock lock(m_mutex);
    if ((int)m_datas.size() <= fd) return;
    m_datas[fd].reset();
}

}  // namespace qc