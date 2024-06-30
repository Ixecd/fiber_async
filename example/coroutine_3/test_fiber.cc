#include <iostream>
#include <thread>
#include "fiber.hpp"

using namespace qc;
using namespace std;

void fiber_func() {
    // get 当前 任务协程
    //Fiber::ptr cur = Fiber::GetThis();
    cout << "[fiber] : " << " running." << endl;
    // cur->yield();
}

void test_fiber() {
    // 线程主协程
    Fiber::ptr cur = Fiber::GetThis();
    cout << "[fiber] : get fiber succ." << endl;
    // 使用智能指针,创建对象就需要使用只能指针的形式
    // Fiber::ptr fiber(new Fiber(fiber_func, 0, true));

    // 
    Fiber *fiber = new Fiber(fiber_func, 0);
    fiber->resume();

    cout << "[main fiber] : " << cur->git_id() << " running." << endl;

}

int main() {
    //cout << "Hello Fiber" << endl;

    test_fiber();
    // thread(test_fiber);

    // while(1);

    return 0;
}