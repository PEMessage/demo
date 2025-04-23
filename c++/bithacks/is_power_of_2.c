#include <stdio.h>
#include <stdbool.h>

// Method 1: Basic power of 2 check (incorrectly considers 0 as power of 2)
bool is_power_of_two_basic(unsigned int v) {
    return (v & (v - 1)) == 0;
}

// Method 2: Corrected power of 2 check (properly handles 0)
bool is_power_of_two_correct(unsigned int v) {
    return v && !(v & (v - 1));
}

void test_power_of_two(unsigned int v) {
    printf("Value: %u\n", v);
    printf("Method 1 (basic):    %s\n", is_power_of_two_basic(v) ? "true" : "false");
    printf("Method 2 (correct):  %s\n", is_power_of_two_correct(v) ? "true" : "false");
    printf("----------------------------\n");
}

int main() {
    printf("Power of 2 Check Demo\n\n");
    
    // Test cases
    test_power_of_two(0);     // Not a power of 2 (special case)
    test_power_of_two(1);     // 2^0 = 1
    test_power_of_two(2);     // 2^1
    test_power_of_two(3);     // Not a power of 2
    test_power_of_two(4);     // 2^2
    test_power_of_two(8);     // 2^3
    test_power_of_two(15);    // Not a power of 2
    test_power_of_two(16);    // 2^4
    test_power_of_two(255);   // Not a power of 2
    test_power_of_two(256);   // 2^8
    test_power_of_two(1024);  // 2^10
    
    return 0;
}
