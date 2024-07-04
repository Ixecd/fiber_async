/**
 * @file thread_pool.hpp
 * @author 线程池
 * @brief 封装线程池
 * @version 0.1
 * @date 2024-07-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "scheduler.hpp" // for run
#include "thread.hpp"
namespace qc {

class thread_pool {
public:
    thread_pool(size_t threads = 1) {
        
    }

private:

    size_t m_size;
    // 要数组并且里面每一个都是智能指针类型
    std::vector<Thread::ptr> m_pool;
    
};


}