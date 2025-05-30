
#include <stdio.h>
#include <assert.h>

// x:     1 1 0 0
// ~x:    0 0 1 1   (bitwise NOT)
// ~x+1:  0 1 0 0   (this is -x)
//         ^
//         This is where the first carry stops
// x:     1 1 0 0
// -x:    0 1 0 0
// AND:   0 1 0 0

unsigned int lowest_bit_mask(unsigned int x) {
    return x & -x;
}
// Step1: Bit Spread Hacks:
//     Original:            00001100
//     After x>>1:          00000110
//     After x|=x>>1:       00001110
//     After x>>2:          00000011
//     After x|=x>>2:       00001111
//     ... (continues for all shifts)
//     Final spread:        00001111
// Step2:
//     x:       00001111
//     x>>1:    00000111
//     ~(x>>1): 11111000
//     AND:     00001000 (8 in decimal)

unsigned int highest_bit_mask(unsigned int x) {
    // Propagate all set bits to the right
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    // Now isolate the highest bit
    return x & ~(x >> 1);
}


int bitmask_to_position(unsigned int mask) {
    // Verify it's a valid bitmask (has exactly one bit set)
    assert(mask != 0 && (mask & (mask - 1)) == 0);

    int pos = 0;
    while ((mask & 1) == 0) {
        mask >>= 1;
        pos++;
    }
    return pos;
}


unsigned int position_to_bitmask(int pos) {
    assert(pos >= 0 && pos < sizeof(unsigned int) * 8);
    return 1u << pos;
}


void demo(unsigned int x) {
    printf("Input: %u (0x%08x)\n", x, x);

    unsigned int lmask = lowest_bit_mask(x);
    printf("Lowest bit mask : 0x%08x\n", lmask);

    unsigned int hmask = highest_bit_mask(x);
    printf("Highest bit mask: 0x%08x\n", hmask);

    if (lmask != 0 && hmask != 0) {
        int lpos = bitmask_to_position(lmask);
        printf("Position of lowest bit : %d\n", lpos);

        int hpos = bitmask_to_position(hmask);
        printf("Position of highest bit: %d\n", hpos);

    } else {
        printf("No bits set (input was zero)\n");
    }

    printf("\n");
}

int main() {
    demo(0);       // No bits set
    demo(1);       // Bit 0 set
    demo(12);      // Binary 1100 (bits 2 and 3 set)
    demo(0x80);    // Only bit 7 set
    demo(0x4000);  // Only bit 14 set
    demo(0x80000000); // Only bit 31 set (on 32-bit systems)

    return 0;
}

