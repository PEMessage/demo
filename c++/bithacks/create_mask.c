#include <stdint.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

uint64_t create_mask(int n) {
    assert(n >= 0 && n < 64);
    if(n == 63) return 0xFFFFFFFFFFFFFFFF;
    return (1ULL << (n + 1)) - 1;
}

int main() {
    printf("Testing create_mask function:\n");

    uint64_t mask0 = create_mask(0);
    printf("create_mask(0) = 0x%016lx\n", mask0);
    assert(mask0 == 0b1);
    uint64_t mask1 = create_mask(1);
    printf("create_mask(1) = 0x%016lx\n", mask1);
    assert(mask1 == 0b11);

    uint64_t mask62 = create_mask(62);
    printf("create_mask(62) = 0x%016lx\n", mask62);
    assert(mask62 == 0x7FFFFFFFFFFFFFFF);

    uint64_t mask63 = create_mask(63);
    printf("create_mask(63) = 0x%016lx\n", mask63);
    assert(mask63 == 0xFFFFFFFFFFFFFFFF); // All bits set

    return 0;
}
