#include "refbase.h"

using namespace OHOS;
using namespace std;

struct A: RefBase {
    A() {
        printf("Create class A: %p\n", this);
    }
    ~A() {
        printf("~ class A: %p\n", this);
    }
};

int main (int argc, char *argv[]) {
    printf("=================\n");
    printf("Create a in Stack\n");
    printf("=================\n");
    A a;
    printf("=================\n");
    printf("Create sptr pa(Create A in heap, and assgin it to pa)\n");
    printf("=================\n");
    sptr<A> pa = sptr<A>::MakeSptr();
    printf("=================\n");
    printf("Create sptr pb\n");
    printf("=================\n");
    sptr<A> pb = pa;

    printf("=================\n");
    printf("return\n");
    printf("=================\n");
    return 0;
}
