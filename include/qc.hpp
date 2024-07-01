/**
 * @file qc.hpp
 * @author qc
 * @brief marco
 * @version 0.1
 * @date 2024-06-30
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

#include <iostream>
#include <stdlib.h>

namespace qc {

// 判断表达式是否为ture原则上不执行函数
// __FILE__ 文件名
// __LINE__ 行号
// __func__ 函数名
// __PRETTY_FUNCTION__ 具体函数,包括模板参数 --> 可以编译反序列化
#define qc_assert(expr)                                                                         \
    do {                                                                                        \
        if (!(expr)) {                                                                          \
            std::cerr << "Assertion failed:  " << #expr << " at " << __FILE__                   \
                      << ":" << __LINE__ << " : " << __func__ << "()" << std::endl;             \
            std::abort();                                                                       \
        }                                                                                       \
    } while (0)                                                                                 \

/**
 * @details 编译器优化  qc_likely(x) -> x 大概率成立(1),优化;
 *                    qc_unlikely(x)-> x大概率不成立(0),优化;
 *   __builtin_expect 是 GCC 和 LLVM 的内建函数，用于提供分支预测的信息
 */

#if defined __GNUC__ || defined __llvm__
#define qc_likely(x) __builtin_expect(!!(x), 1)
#define qc_unlikely(x) __builtin_expect(!!(x), 0)
#else
#define qc_likely(x) (x)
#define qc_unlikely(x) (x)
#endif

}