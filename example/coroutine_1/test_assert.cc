#include "qc.hpp"

using namespace qc;
using namespace std;

int main() {

    int a = 1;

    qc_assert(a);

    cout << "a = " << a << endl;

    return 0;
}