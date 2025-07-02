// q-gcc: -std=c++20 --
// ========================================
// !!! WE NEED C++20 !!!!
// ========================================
#include <stddef.h>
#include <stdio.h>
#include <iostream>
using namespace std;

// See: 如何优雅的实现C++编译期静态反射 - netcan的文章 - 知乎
//      https://zhuanlan.zhihu.com/p/165993590
// Also: cpreprocessor/get_nth_args.c in C

struct AnyType {
    template <typename T>
    operator T();
}; // AnyType声明了类型转换操作符（《C++ Modern design》书中的术语是稻草人函数），可以转换成任意类型

template <typename T>
consteval size_t CountMember(auto&&... Args) {
    if constexpr (! requires { T{ Args... }; }) { // (2)
                                                  //  T{AnyType{}, AnyType{} ... AnyType{} }  超过元素个数的时候
        return sizeof...(Args) - 1;
    } else {
        return CountMember<T>(Args..., AnyType{}); // (1)  // 不断用anytype 初始化对象
                                                   // T{}
                                                   // T{AnyType{}, }
                                                   // T{AnyType{}, AnyType{} }
                                                   // T{AnyType{}, AnyType{}, AnyType{} }
                                                   // ...
    }
}


template <typename ...Args>
consteval size_t CountArgs(Args&& ...) {
    return sizeof...(Args);
}

template <typename ...Args>
void LoopPrintArgs(Args&& ... args) {
    (
      (std::cout << "   |" << args << std::endl),
      ...
    );
    // Equivalent to: (cout << arg1 << endl), (cout << arg2 << endl), ...
}

int main(int argc, char** argv) {
    struct Test { int a; int b; int c; int d; };
    printf("This is %zu args\n", CountArgs(1, 2, 3, 4, 5));
    printf("Test struct has %zu member\n", CountMember<Test>());
    printf("LoopPrintArgs: \n");
    {
        LoopPrintArgs(6, 7, 8);
    }
}
