/**
 * @file noncopyable.hpp
 * @author qc
 * @brief
 * @version 0.1
 * @date 2024-06-30
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

namespace qc {

/// @brief 不可拷贝类型
class Noncopyable {
public:
    Noncopyable() = default;

    ~Noncopyable() = default;

    Noncopyable(const Noncopyable&) = delete;

    Noncopyable& operator=(const Noncopyable&) = delete;
    
    Noncopyable(const Noncopyable&&) = delete;

    Noncopyable& operator=(const Noncopyable&&) = delete;
};

}  // namespace qc