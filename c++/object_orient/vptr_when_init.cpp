#include <iostream>
#include <string>
using namespace std;

struct A {
    int a{};
    string str1{};
    A() { cout << "A() constructor called" << endl; Bar(); }
    virtual ~A() { Bar(); cout << "~A() destructor called" << endl; }
    virtual void Foo() { cout << "  A::Foo()" << endl; }
    void Bar() { Foo(); }
};

struct B : A {
    int b{};
    string str2{};
    B() {  cout << "B() constructor called" << endl; Bar(); }
    virtual ~B() { Bar(); cout << "~B() destructor called" << endl; }
    virtual void Foo() { cout << "  B::Foo()" << endl; }
};

struct C : B {
    int c{};
    string str3{};
    C() {  cout << "C() constructor called" << endl; Bar(); }
    virtual ~C() { Bar(); cout << "~C() destructor called" << endl; }
    virtual void Foo() { cout << "  C::Foo()" << endl; }
};

// 当继承初始化时候，初始化到哪个类，vptr设置成当前类的vtable
// C++ Under the Hood: Internal Class Mechanisms
// See: https://www.youtube.com/watch?v=gWinNE5rd6Q&t=728s
int main() {
    C cObj;
    // Output:
    //
    // A() constructor          V  A::A() : a(0), str1(), __v_table_ptr(A::'v_table'), { Bar(); ... }
    //   A::Foo()               |      now __v_table_ptr set to a
    // B() constructor called   |  B::B() : A(), b(0), str2(), __v_table_ptr(B::'v_table'), { Bar(); ... }
    //   B::Foo()               |  ...
    // C() constructor called   |  C::C() : B(), c(0), str3(), __v_table_ptr(C::'v_table'), { Bar(); ... }
    //   C::Foo()               V  ...
    cout << "===== Obj create complete ====" << endl;
    //   C::Foo()               V
    // ~C() destructor called   |  C::~C() : __v_table_ptr(C::'v_table'), { Bar(); ... }, ~str3(), ~B()
    //   B::Foo()               |
    // ~B() destructor called   |  B::~B() : __v_table_ptr(B::'v_table'), { Bar(); ... }, ~str2(), ~A()
    //   A::Foo()               |
    // ~A() destructor called   V  A::~A() : __v_table_ptr(A::'v_table'), { Bar(); ... }, ~str1()
    return 0;
}
