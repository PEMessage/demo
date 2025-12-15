#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

// 【如果直接用 C 语言替代正则表达式会怎样 | Tsoding Daily】 【精准空降到 12:55】
// https://www.bilibili.com/video/BV1MrmjBjEeg/?share_source=copy_web&vd_source=ba726eaf572f03aca4ba3d79f0118159&t=775

bool is_huid(const char *s)
{
    // similiar to '{0-9}[8]-{0-9}[6]'
    //
    // it just work for null delim,
    // if meet '\0', it just work, 'isdigit' will be false
    // we pack 2 action into *s++
    //  1. advance string
    //  2. return current one
    for (size_t i = 0; i < 8; ++i) if (!isdigit(*s++)) return false;
    if (*s++ != '-') return false;
    for (size_t i = 0; i < 6; ++i) if (!isdigit(*s++)) return false;
    return true;
}

#define HUID_TEST_CASE(huid) printf("%s => %s\n", (huid), is_huid(huid) ? "true" : "false");

int main()
{
    HUID_TEST_CASE("20250906-014343");
    HUID_TEST_CASE("HelloWorld");
    HUID_TEST_CASE("20251011-061535");
    HUID_TEST_CASE("20251a11-061535");
    HUID_TEST_CASE("");
    return 0;
}
