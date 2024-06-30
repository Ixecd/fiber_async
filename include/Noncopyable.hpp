/**
 * @file Noncopyable.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2024-06-30
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

namespace qc {

class Noncopyable {
private:
    Noncopyable() {}
    Noncopyable& operator=(const Noncopyable&) = delete;
    Noncopyable(const Noncopyable&) = delete;
};

}  // namespace qc