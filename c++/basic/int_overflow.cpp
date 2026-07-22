#include <iostream>
#include <limits>
#include <cstdint>

int main() {
    constexpr int MAX_INT = std::numeric_limits<int>::max();
    constexpr int MIN_INT = std::numeric_limits<int>::min();

    std::cout << "int max:  " << MAX_INT << '\n';
    std::cout << "int min:  " << MIN_INT << '\n';
    std::cout << "---\n";

    volatile int x = MAX_INT;

    int r_max_plus_max = x + x;
    std::cout << "MAX + MAX = " << x << " + " << x << " = " << r_max_plus_max << '\n';

    int r_max_plus_1 = x + 1;
    std::cout << "MAX + 1   = " << x << " + 1 = " << r_max_plus_1 << '\n';

    int r_max_plus_2 = x + 2;
    std::cout << "MAX + 2   = " << x << " + 2 = " << r_max_plus_2 << '\n';
}
