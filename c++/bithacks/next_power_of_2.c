
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

// Thanks to: https://graphics.stanford.edu/~seander/bithacks.html
// Thanks to: https://www.zhihu.com/question/27417946/answer/1253126563

typedef uint32_t (*power_func)(uint32_t);

uint32_t next_power_of_2(uint32_t x) {
    uint32_t y = 1;
    while (y < x) y *= 2;
    return y;
}

uint32_t quick_next_power_of_2(uint32_t x) {
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
    return x;
}

long long get_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

void benchmark(const char* name, power_func func) {
    const int NUM_ITERATIONS = 1000000;
    uint32_t test_cases[] = {0, 1, 2, 3, 5, 8, 10, 16, 20, 31, 32, 33, 100, 1000};
    const int NUM_TEST_CASES = sizeof(test_cases) / sizeof(uint32_t);

    long long start = get_timestamp();

    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        for (int i = 0; i < NUM_TEST_CASES; i++) {
            volatile uint32_t result = func(test_cases[i]);
            (void)result; // Prevent optimization
        }
    }

    long long end = get_timestamp();
    printf("%-20s: %lld us\n", name, end - start);
}

int main() {
    // Cast function pointers to match the typedef
    benchmark("next_power_of_2      ",       next_power_of_2);
    benchmark("quick_next_power_of_2", quick_next_power_of_2);

    return 0;
}
