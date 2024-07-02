#include <iostream>
#include <list>

using namespace std;

int main() {

    list<int> _queue;
    _queue.push_back(1);
    _queue.push_back(2);

    cout << "_queue.size() = " << _queue.size() << endl;

    return 0;
}