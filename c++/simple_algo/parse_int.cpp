
#include <string_view>
#include <sstream>  // for std::istringstream

// See: https://www.youtube.com/watch?v=n1sJtsjbkKo&t=793s
// Modern C++ Error Handling - Phil Nash - CppCon 2024

// similar behavior to istream
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

    printf("Using parse_int:\n");
    printf("%d\n", parse_int("42"));  // 42
    printf("%d\n", parse_int("42x")); // 42
    printf("%d\n", parse_int("x42")); // 0

    printf("\n");

    printf("Using istringstream:\n");
    auto try_parse = [](const std::string& s) {
        std::istringstream iss(s);
        int value;
        iss >> value;
        return value;
    };

    printf("%d\n", try_parse("42"));  // 42
    printf("%d\n", try_parse("42x")); // 42
    printf("%d\n", try_parse("x42")); // 0

    return 0;
}
