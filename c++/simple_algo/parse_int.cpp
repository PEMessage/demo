
#include <iostream>
#include <string_view>
#include <string>

// See: https://www.youtube.com/watch?v=n1sJtsjbkKo&t=793s
// Modern C++ Error Handling - Phil Nash - CppCon 2024

int parse_int(std::string_view number) {
    int acc = 0;
    for (char c : number) {
        if (c < '0' || c > '9') {
            return acc;
        }
        acc *= 10;
        acc += c - '0';
    }
    return acc;
}

int main() {

    printf("%d\n", parse_int("42"));
    printf("%d\n", parse_int("42x"));
    printf("%d\n", parse_int("x42"));
    return 0;
}
