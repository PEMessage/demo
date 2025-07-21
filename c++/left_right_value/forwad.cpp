#include <iostream>
#include <utility>

// See: https://www.cnblogs.com/yinheyi/p/14853787.html
// See: https://changkun.de/modern-cpp/zh-cn/03-runtime/

void reference(int& v) {
    std::cout << "lvalue" << std::endl;
}
void reference(int&& v) {
    std::cout << "rvalue" << std::endl;
}

// 1. pass_uniref
// pass_uniref, accept both lvalue/right value, but args is lvalue inside v
// ====================================================================================
template <typename T>
void pass_uniref(T&& v) {
    std::cout << "pass_uniref -> reference(...) => ";
    reference(v);
}

// 2. pass_overload
// pass_overload, accept both lvalue/right value, and we correct forward, but duplicate
// ====================================================================================
template <typename T>
void pass_overload(T& v) {
    std::cout << "pass_overload(T&) -> reference(...) => ";
    reference(v);
}

template <typename T>
void pass_overload(T&& v) {
    std::cout << "pass_overload(T&&) -> move -> reference(...) => ";
    reference(std::move(v));
}

// 2. Pefect forward
// best way, only write once, uni reference + std::forward
// ====================================================================================
template <typename T>
void pass_perfect(T&& v) {
    std::cout << "pass_perfect(T&&) -> forward -> reference(...) => ";
    reference(std::forward<T>(v));
}

int main() {
    {
        std::cout << "Pass rvalue => ";
        pass_uniref(1);

        std::cout << "Pass lvalue => ";
        int l = 1;
        pass_uniref(l);
    }

    {
        std::cout << "Pass rvalue => ";
        pass_overload(1);

        std::cout << "Pass lvalue => ";
        int l = 1;
        pass_overload(l);
    }

    {
        std::cout << "Pass rvalue => ";
        pass_perfect(1);

        std::cout << "Pass lvalue => ";
        int l = 1;
        pass_perfect(l);
    }


    return 0;
}
