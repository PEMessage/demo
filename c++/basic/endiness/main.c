#include <endian.h>
#include <stdint.h>
#include <stdio.h>

#define IS_LITTLE_ENDIAN ({ \
    union { \
        uint16_t word; \
        uint8_t bytes[2]; \
    } test = { .word = 0x0001 }; \
    test.bytes[0]; \
})

void htobe(void *ptr, size_t size) {
    if (size <= 1 || !IS_LITTLE_ENDIAN) return;

    uint8_t *bytes = (uint8_t *)ptr;
    uint8_t temp;

    // Reverse the byte order
    for (size_t i = 0; i < size / 2; i++) {
        temp = bytes[i];
        bytes[i] = bytes[size - 1 - i];
        bytes[size - 1 - i] = temp;
    }
}

int main(int argc, char *argv[])
{
    printf("Endianness: %d (1 = Little Endian, 0 = Big Endian)\n", IS_LITTLE_ENDIAN);
    uint32_t a = 0xDEADBEEF;
    printf("a is 0x%X\n", a);

    htobe(&a, sizeof(a));
    printf("a is 0x%X\n", a);
    return 0;
}


