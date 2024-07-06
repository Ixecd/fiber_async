/**
 * @file singleton.hpp
 * @author qc
 * @brief 单例模式封装
 * @version 0.1
 * @date 2024-06-30
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

#include <memory>

namespace qc {

// 懒汉式 C11之前用的是new要考虑线程安全问题
template<class T, class X = void, int N = 0>
class Singleton {
public:
    static T* GetInstance() {
        static T v;
        return &v;
    }
};

// 饿汉式
template <class T>
class ESingleton {
    static T* GetInstance() {
        return &v;
    }
private:
    static T v;
};

template <class T>
T ESingleton<T>:: v;

template<class T, class X = void, int N = 0>
class SingletonPtr {
public:
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
    }
};

}