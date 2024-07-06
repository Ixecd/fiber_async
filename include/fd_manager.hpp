/**
 * @file fd_manager.hpp
 * @author qc
 * @brief 文件句柄上下文,管理文件句柄类型,阻塞,关闭,读写超时
 * @version 0.1
 * @date 2024-07-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <memory>
#include <vector>
#include "mutex.hpp"
#include "singleton.hpp"
#include "thread.hpp"

namespace qc {

class FdCtx : public std::enable_shared_from_this<FdCtx> {
public:
    typedef std::shared_ptr<FdCtx> ptr;

    FdCtx(int fd);
    ~FdCtx();

    bool isInit() const { return m_isInit; }
    bool isSocket() const { return m_isSocket; }
    bool isClose() const { return m_isClosed; }
    
    // 用户主动设置非阻塞
    void setUserNonblock(bool v) { m_userNonblock = v; }

    bool getUserNonblock() const { return m_userNonblock; }

    void setSysNonblock(bool v) { m_sysNonblock = v; }

    bool getSysNonblock() const { return m_sysNonblock; }

    void setTimeout (int type, uint64_t v);

    uint64_t getTimeout(int type);
private:

    bool init();

private:
    /// @brief 是否初始化
    bool m_isInit : 1;
    /// @brief 是否为socket
    bool m_isSocket : 1;
    /// @brief 是否hook非阻塞
    bool m_sysNonblock : 1;
    /// @brief 是否用户主动设置非阻塞
    bool m_userNonblock : 1;
    /// @brief 是否关闭
    bool m_isClosed : 1;
    /// @brief 文件句柄
    int m_fd;
    /// @brief 读超时时间毫秒
    uint64_t m_recvTimeout;
    /// @brief 写超时时间毫秒
    uint64_t m_sendTimeout;

};

class FdManager {
public:
    typedef RWMutex RWMutexType;

    FdManager();

    FdCtx::ptr get(int fd, bool auto_create = false);

    void del(int fd);
private:
    RWMutexType m_mutex;

    std::vector<FdCtx::ptr> m_datas;
};

// 文件句柄单例模式
typedef Singleton<FdManager> FdMgr;

}