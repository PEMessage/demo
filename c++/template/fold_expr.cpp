#include <iostream>

// 折叠表达式（Fold Expressions ）是 C++17 引入的一项强大特性 
// ，专门用于在可变参数模板中简洁地处理参数包（parameter packs） 。
// 它特别适合对一组参数进行逐个操作并合并结果 的场景。
using namespace std;
// 近距离接触C++折叠表达式 - Coder Arthur的文章 - 知乎
// https://zhuanlan.zhihu.com/p/690794532
// Using https://cppinsights.io/
//
// Also:
// https://www.zhihu.com/question/813637500/answer/6540279171
template <typename... Args>
auto LeftFold(Args... args)
{
    return (... + args); //  ((__args0 + __args1) + __args2) + __args3;

}

template <typename... Args>
auto RightFold(Args... args)
{
    return (args + ...); // __args0 + (__args1 + (__args2 + __args3));
}




int test1()
{ 
    cout << RightFold(1, 2, 3, 4) << endl;
    cout << LeftFold(1, 2, 3, 4) << endl;

    return 1;
};


template<typename... Args>
auto LeftFoldSum(Args&&... args)
{
    return (5 + ... + args); // (((5 + __args0) + __args1) + __args2) + __args3;
}

template<typename... Args>
auto RightFoldSum(Args&&... args)
{
    return (args + ... + 5); // __args0 + (__args1 + (__args2 + (__args3 + 5)));
}

int test2()
{ 
    cout << RightFoldSum(1, 2, 3, 4) << endl;
    cout << LeftFoldSum(1, 2, 3, 4) << endl;

    return 1;
};


int main (int argc, char *argv[]) {
    test1();
    test2();
    return 0;
}
