#include "refbase.h"
#include "stdio.h"
#include <iostream>

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

#define PRT(code) \
    std::cout << "\033[1;34m" << #code << "\033[0m" << std::endl; \
    code

#define header() std::cout \
    << "\033[1;31m" \
    << "--------------------" << std::endl \
    << __func__ << std::endl \
    << "--------------------" << std::endl \
    << "\033[0m"

void test_normal() {
    header();
    PRT(sptr<A> pa = sptr<A>::MakeSptr();)
    PRT(sptr<A> pb = pa;)
    PRT(cout << "PB SptrRefCount: " <<  pb->GetSptrRefCount() << endl);
    PRT(cout << "PB WptrRefCount: " <<  pb->GetWptrRefCount() << endl);

    PRT(pb = NULL;)
    PRT(return;)
}

void test_swap() {
    header();
    PRT(sptr<A> pa = sptr<A>::MakeSptr();)
    PRT(sptr<A> pb = sptr<A>::MakeSptr();)
    PRT(sptr<A> pc = pa;)
    PRT(pc = pb;)
    PRT(return;)
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
    test_swap();
    // test_create_in_stack(); // THIS will cause dounle free
    return 0;
}
