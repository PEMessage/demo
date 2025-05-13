#include "refbase.h"
#include <iostream>

using namespace OHOS;
using namespace std;

struct A: RefBase {
    A() {
        cout << "Create class A: "  << this << endl;
    }
    ~A() {
        cout << "~ class A: "  << this << endl;
    }
};

int main (int argc, char *argv[]) {
    cout << "=================" << endl;
    cout << "Create a in Stack" << endl;
    cout << "=================" << endl;
    A a;
    cout << "=================" << endl;
    cout << "Create sptr pa" << endl;
    cout << "=================" << endl;
    sptr<A> pa = sptr<A>::MakeSptr();
    cout << "=================" << endl;
    cout << "Create sptr pb" << endl;
    cout << "=================" << endl;
    sptr<A> pb = pa;

    cout << "=================" << endl;
    cout << "return" << endl;
    cout << "=================" << endl;
    return 0;
}
