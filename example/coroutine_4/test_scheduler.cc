#include "fiber.hpp"
#include "scheduler.hpp"

using namespace std;
using namespace qc;

void test_fiber_1() {
    cout << "test_fiebr_1 begin" << endl;
    cout << "test_fiber_1 end" << endl;
}

void test_fiber_2() {
    cout << "test_fiber_2 begin" << endl;
    cout << "test_fiber_2 end" << endl;
}

void test_fiber_3() {
    cout << "test_fiebr_3 begin" << endl;
    cout << "test_fiber_3 end" << endl;
}


int main() {
    cout << "main begin()" << endl;
    Scheduler sche(1);

    sche.add_task(test_fiber_1);
    sche.add_task(test_fiber_2);

    Fiber::ptr fiber(new Fiber(&test_fiber_3));

    sche.add_task(fiber);

    sche.start();

    sche.stop();

    std::cout << "main end()" << std::endl;
    return 0;
}