

#include <type_traits>
#include <iostream>

// Macro to check if an expression is an lvalue (returns bool)
#define IS_LVALUE(expr) std::is_lvalue_reference<decltype((expr))>::value

// Macro to check if an expression is an rvalue (returns bool)
#define IS_RVALUE(expr) (!std::is_reference<decltype((expr))>::value)

// Macro to print whether an expression is an lvalue or rvalue (for debugging)
#define PRINT_VALUE_CATEGORY(expr) \
    std::cout << "Expression lvalue: '" << #expr << "' is a " \
              << (IS_LVALUE(expr) ? "true" : "false") \
              << std::endl

int main() {
    int x = 42;
    int& lref = x;
    int&& rref = 100;

    PRINT_VALUE_CATEGORY(x);       // lvalue
    PRINT_VALUE_CATEGORY(lref);    // lvalue
    PRINT_VALUE_CATEGORY(rref);    // lvalue (named rvalue reference is an lvalue)
    PRINT_VALUE_CATEGORY(123);     // rvalue
    PRINT_VALUE_CATEGORY(x + 1);   // rvalue
    PRINT_VALUE_CATEGORY(std::move(x)); // rvalue

    // Compile-time checks (usable in static_assert)
    static_assert(IS_LVALUE(x), "x must be an lvalue");
    static_assert(!IS_LVALUE(42), "42 must be an rvalue");
}
