#include <stdio.h>
// https://netcan.github.io/2020/08/01/如何优雅的实现C-编译期静态反射/
#define GET_NTH_ARG(                                                                        \
    _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9,  _10, _11, _12, _13, _14, _15, _16,         \
    _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,         \
    _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48,         \
    _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, n, ...) n

#define GET_ARG_COUNT(...) GET_NTH_ARG(__VA_ARGS__,                     \
        64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, \
        48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, \
        32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 23, 21, 20, 19, 18, 17, \
        16, 15, 14, 13, 12, 11, 10, 9,  8,  7,  6,  5,  4,  3,  2,  1)

int main() {
    int a,b,c,d,e;
    printf("Number of arguments: %d\n", GET_ARG_COUNT(a));               // 1
    printf("Number of arguments: %d\n", GET_ARG_COUNT(a, b));            // 2
    printf("Number of arguments: %d\n", GET_ARG_COUNT(a, b, c));         // 3
    printf("Number of arguments: %d\n", GET_ARG_COUNT(a, b, c, d));      // 4
    printf("Number of arguments: %d\n", GET_ARG_COUNT(a, b, c, d, e));   // 5

    // 测试更多参数
    printf("Number of arguments: %d\n",
        GET_ARG_COUNT(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));                  // 10

    return 0;
}





