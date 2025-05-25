#include <iostream>

using namespace std;
// 近距离接触C++折叠表达式 - Coder Arthur的文章 - 知乎
// https://zhuanlan.zhihu.com/p/690794532
// Using https://cppinsights.io/
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
