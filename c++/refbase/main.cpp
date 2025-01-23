

#include "refbase.h"
#include <iostream>



using namespace OHOS;
using namespace std;

struct A: RefBase {
    A() {
        cout << "Create class A: "  << this << endl;
    }
};

int main (int argc, char *argv[]) {
    A a;
    
    return 0;
}
