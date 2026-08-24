// ============================================================================
// Summary of implicit-conversion behavior under different compiler flags
// (verified empirically with GCC)
//
// Build variants:
//   A) g++ -std=c++11 narrow.cpp                    // default: only -Wnarrowing (braces) errors
//   B) g++ -std=c++11 -Werror=conversion narrow.cpp
//   C) g++ -std=c++11 -Werror=sign-conversion narrow.cpp
//   D) g++ -std=c++11 -Werror=narrowing narrow.cpp  // braces-initialization only
//
// "error" = compilation fails under that flag; "ok" = compiles successfully.
// ----------------------------------------------------------------------------
// | Conversion example       | Category            | A def | B conv | C sign | D narrow |
// |--------------------------|---------------------|------|--------|--------|----------|
// | int   -> double (widen)  | safe widening       | ok   | ok     | ok     | ok       |
// | short -> int   (widen)   | safe widening       | ok   | ok     | ok     | ok       |
// | int   -> short (narrow)  | width narrowing     | ok   | error  | ok     | ok       |
// | int   -> char  (narrow)  | width narrowing     | ok   | error  | ok     | ok       |
// | double-> int  (real->int)| real/integer        | ok   | error  | ok     | ok       |
// | double-> float (prec.)   | float precision     | ok   | error* | ok     | ok       |
// | int   -> unsigned (sign) | signed/unsigned     | ok   | ok     | error  | ok       |
// | unsigned-> int  (sign)   | signed/unsigned     | ok   | ok     | error  | ok       |
// | int x{3.5}; (braces)     | list-init narrowing | error| error  | error  | error    |
// ----------------------------------------------------------------------------
// * In B, double->float is flagged by -Wfloat-conversion (enabled by -Wconversion).
// * Brace-list narrowing is governed by -Wnarrowing, which GCC treats as an
//   error by default regardless of the chosen flag, so that row errors in all
//   columns (A/B/C/D).
//
// Takeaways:
//   - -Wconversion catches "implicit conversions that may change a value"; it is
//     neither "every conversion" nor purely "narrowing".
//   - Safe widening (int->double) is NOT caught; sign conversions are NOT caught
//     by default (need -Wsign-conversion).
//   - -Wnarrowing is the one specifically about brace {} list-init narrowing
//     (ill-formed at the standard level), and it errors by default.
// ============================================================================

#include <iostream>

int main() {
    // Compiles fine by default: these implicit narrowings are not warned by GCC.
    double d = 3.5;
    int x = d;            // double -> int  (narrowing, no warning by default)
    int big = 1000;
    char c = big;         // int -> char    (narrowing, no warning by default)
    (void)x;
    (void)c;
    std::cout << x << " " << (int)c << "\n";
    return 0;
}
