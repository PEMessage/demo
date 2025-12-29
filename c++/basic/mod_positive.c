#include <stdio.h>

#define MOD_POSITIVE(value, base) \
    (((value) % (base) + (base)) % (base))

#define MOD_WITH_OFFSET(value, base, offset) \
    (MOD_POSITIVE((value) - (offset), base) + (offset))


void test_mod_positive(int value, int base) {
    printf("MOD_POSITIVE: %d %% %d = %d\n",
            value, base,
            MOD_POSITIVE(value, base)
            );
}

void test_mod_with_offset(int value, int base, int offset) {
    printf("MOD_OFFSET: value: %d, base: %d, offset: %d = %d\n",
            value, base, offset,
            MOD_WITH_OFFSET(value, base, offset)
            );
}


int main(int argc, char *argv[])
{
    printf("=== Testing MOD_POSITIVE ===\n");
    test_mod_positive(5, 3);      // 5 % 3 = 2
    test_mod_positive(-5, 3);     // (-5 % 3 + 3) % 3 = 1
    test_mod_positive(17, 5);     // 17 % 5 = 2
    test_mod_positive(-17, 5);    // (-17 % 5 + 5) % 5 = 3
    test_mod_positive(10, 10);    // 10 % 10 = 0
    test_mod_positive(-10, 10);   // (-10 % 10 + 10) % 10 = 0

    printf("\n=== Testing MOD_WITH_OFFSET ===\n");

    // Example 1: Array indexing with negative values
    // Base: 5 elements, offset: 0 (standard array indices 0-4)
    test_mod_with_offset(7, 5, 0);    // Wraps 7 to 2 (0-4 range)
    test_mod_with_offset(-3, 5, 0);   // Wraps -3 to 2 (0-4 range)

    // Example 2: Circular buffer with different base
    test_mod_with_offset(12, 8, 0);   // Wraps 12 to 4 (0-7 range)
    test_mod_with_offset(-4, 8, 0);   // Wraps -4 to 4 (0-7 range)

    // Example 3: With non-zero offset
    // Useful for wrapping values to a specific range
    test_mod_with_offset(25, 10, 5);  // Wraps to range 5-14: 25 -> 5
    test_mod_with_offset(3, 10, 5);   // Wraps to range 5-14: 3 -> 13
    test_mod_with_offset(15, 10, 5);  // Wraps to range 5-14: 15 -> 5

    // Example 4: Negative offset
    test_mod_with_offset(7, 5, -2);   // Wraps to range -2 to 2: 7 -> -2
    test_mod_with_offset(-5, 5, -2);  // Wraps to range -2 to 2: -5 -> 0

    // Example 5: Wrapping angles or other periodic values
    // Wrap degrees to 0-359 range
    test_mod_with_offset(370, 360, 0);    // 370° -> 10°
    test_mod_with_offset(-10, 360, 0);    // -10° -> 350°

    // Example 6: Wrap to 1-based indexing (1-12 months)
    test_mod_with_offset(13, 12, 1);      // Month 13 -> January (1)
    test_mod_with_offset(0, 12, 1);       // Month 0 -> December (12)
    test_mod_with_offset(-1, 12, 1);      // Month -1 -> November (11)


    return 0;
}
