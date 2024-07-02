#include <bits/stdc++.h>

using namespace std;

void run(int& a, int& b) { cout << "start run" << endl; }

int main() {
    int x = 10;
    int y = 20;
    /**
     * @brief 下面两种效果一摸一样,按自己习惯来,bind(函数地址,参数)->得到的是std::function<void()>类型
     */
    function<void()> cb_1 = bind(&run, x, y);
    function<void()> cb_2 = bind(run, x, y);

    cb_1();
    cb_2();

    cout << "main end" << endl;

    return 0;
}