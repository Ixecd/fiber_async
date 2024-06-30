#include <iostream>
#include <thread>

#include "mutex.hpp"
#include "qc.hpp"

using namespace qc;
using namespace std;

void thread_func(RWMutex& rw_mutex, int id) {
    WriteScopedLockImpl<RWMutex> lock(rw_mutex);
    cout << "Thread " << id << " acquired the lock." << endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cout << "Thread " << id << " released the lock." << endl;
}

void test_RWMutex() {
    RWMutex rw_mutex;
    // 下面使用std::thread来创建线程
    std::thread t1(thread_func, std::ref(rw_mutex), 1);
    std::thread t2(thread_func, std::ref(rw_mutex), 2);

    // t1.join();
    // t2.join();
    t1.detach();
    t2.detach();
}

void threadFunction(int id) {
    for (int i = 0; i < 5; ++i) {
        std::cout << "Thread " << id << "is running" << endl;
        std::this_thread::yield();
    }
}

int main() {
    // test_RWMutex();
    // 主线程退出会导致整个程序结束，包括所有正在运行的线程。因此，通常需要确保主线程不会在所有分离的线程完成之前退出，以确保所有后台工作能够正确完成。
    // while(1);

    std::vector<std::thread> threads;

    for (int i = 0; i < 3; ++i) {
        threads.emplace_back(threadFunction, i);
    }

    for (auto& t : threads) t.join();

    return 0;
}