#include "refbase.h"
#include "stdio.h"

using namespace OHOS;
using namespace std;

struct A: RefBase {
    A() {
        printf("|  Create class A: %p\n", this);
    }
    ~A() {
        printf("|  ~ class A: %p\n", this);
    }
};


void test_normal() {
    printf("--------------------------------------\n");
    printf("Func: %s\n", __func__);
    printf("--------------------------------------\n");
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
    sptr<A> pc = pa;

    printf("=================\n");
    printf("return\n");
    printf("=================\n");
}

void test_create_in_stack() {
    // if you create some refbase in stack, it should never be !!!never!!! mangered by sptr
    A a;
    {
        sptr<A> pa = &a;
        
    } // pa delete, object 'a' do not have any strong ref now, so delete a
} // double free HERE

int main (int argc, char *argv[]) {
    test_normal();
    // test_create_in_stack(); // THIS will cause dounle free
    return 0;
}
