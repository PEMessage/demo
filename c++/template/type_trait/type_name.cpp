#include <cstdlib>

#include <iostream>

// See: https://stackoverflow.com/questions/81870/is-it-possible-to-print-the-name-of-a-variables-type-in-standard-c/56766138#56766138
#define CXX17_WAY
#if defined(CXX11_WAY)
    #include <type_traits>
    #include <typeinfo>
    #include <string>
    // Why reference is not pointer ?
    // My think: && / & more like const vs volatile
    //           and pointer is unlike them(already different by typeid, C already exist)
    template <class T>
    std::string type_name() {
        using TR = typename std::remove_reference<T>::type;
        std::string r = typeid(TR).name();
        if (std::is_const<TR>::value)
            r += " const";
        if (std::is_volatile<TR>::value)
            r += " volatile";
        if (std::is_lvalue_reference<T>::value)
            r += "&";
        else if (std::is_rvalue_reference<T>::value)
            r += "&&";
        return r;
    }

#elif defined(CXX17_WAY)
    #include <string_view>

    template <typename T>
    constexpr auto type_name() {
        std::string_view name, prefix, suffix;
    #ifdef __clang__
        name = __PRETTY_FUNCTION__;
        prefix = "auto type_name() [T = ";
        suffix = "]";
    #elif defined(__GNUC__)
        name = __PRETTY_FUNCTION__;
        prefix = "constexpr auto type_name() [with T = ";
        suffix = "]";
    #elif defined(_MSC_VER)
        name = __FUNCSIG__;
        prefix = "auto __cdecl type_name<";
        suffix = ">(void)";
    #endif
        name.remove_prefix(prefix.size());
        name.remove_suffix(suffix.size());
        return name;
    }
#endif


int& foo_lref();
int&& foo_rref();
int foo_value();


#define PRINT_TYPE(expr) \
    std::cout << "decltype(" #expr ") is: " << type_name<decltype(expr)>() << std::endl

// decltype rule: https://zhuanlan.zhihu.com/p/152154499
// 1. exp 是标识符、类访问表达式，decltype(exp) 和 exp 的类型一致。
// 2. exp 是函数调用，decltype(exp) 和返回值的类型一致。
// 3. exp 若 exp 是一个左值，则 decltype(exp) 是 exp 类型的左值引用, 则和exp 类型一致
int main() {
    int i = 0;
    const int ci = 0;

    std::cout << "decltype(i) is " << type_name<decltype(i)>() << '\n'; // due to rule1
    std::cout << "decltype(ci) is " << type_name<decltype(ci)>() << '\n'; // due to rule1
    std::cout << "decltype((i)) is " << type_name<decltype((i))>() << '\n';  // due to rule3
    std::cout << "decltype((ci)) is " << type_name<decltype((ci))>() << '\n'; // due to rule3

    // due to rule2
    std::cout << "decltype(foo_lref()) is " << type_name<decltype(foo_lref())>() << '\n';
    std::cout << "decltype(foo_rref()) is " << type_name<decltype(foo_rref())>() << '\n';
    std::cout << "decltype(foo_value()) is " << type_name<decltype(foo_value())>() << '\n';

    // due to rule3
    std::cout << "decltype(static_cast<int&>(i)) is " << type_name<decltype(static_cast<int&>(i))>() << '\n';
    std::cout << "decltype(static_cast<int&&>(i)) is " << type_name<decltype(static_cast<int&&>(i))>() << '\n';
    std::cout << "decltype(static_cast<int>(i)) is " << type_name<decltype(static_cast<int>(i))>() << '\n';

    // universal reference: https://zhuanlan.zhihu.com/p/99524127
    // 如果一个变量或者参数被声明为T&&，其中T是被推导的类型，那这个变量或者参数就是一个universal reference。
    auto &&j = 0;
    PRINT_TYPE(j);
    auto &&k = i;
    PRINT_TYPE(k);

}

