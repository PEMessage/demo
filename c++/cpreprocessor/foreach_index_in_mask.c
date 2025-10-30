#include <stdio.h>

#define FOREACH_INDEX_IN_MASK(mask, _i) \
    for (unsigned int _i = 0, _temp_mask = (mask); \
         _temp_mask != 0; \
         _temp_mask >>= 1, _i++) \
        if (_temp_mask & 1)

// Function to print set bits in a mask
void print_set_bits(unsigned int mask, const char* mask_name) {
    printf("%s: 0x%X\n", mask_name, mask);
    printf("Set bits: ");
    FOREACH_INDEX_IN_MASK(mask, index) {
        printf("%u ", index);
    }
    printf("\n\n");
}

int main() {
    printf("Testing FOREACH_INDEX_IN_MASK macro with functions:\n\n");

    // Test case 1: Basic mask with multiple bits set
    print_set_bits(0b1011, "Mask 1 (binary: 1011)");

    // Test case 2: Single bit set
    print_set_bits(0b10000, "Mask 2 (binary: 10000)");

    // Test case 3: All bits set in a small mask
    print_set_bits(0b111, "Mask 3 (binary: 111)");

    // Test case 4: No bits set (edge case)
    print_set_bits(0, "Mask 4 (no bits set)");

    // Test case 5: Larger mask
    print_set_bits(0xA5, "Mask 5 (binary: 10100101)");

    // Additional test cases
    print_set_bits(0b11001100, "Mask 6 (binary: 11001100)");
    print_set_bits(0xFFFF, "Mask 7 (all bits 0-15 set)");

    return 0;
}

