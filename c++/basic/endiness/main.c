#include <stdint.h>
#include <stdio.h>

#define IS_LITTLE_ENDIAN ({ \
    union { \
        uint16_t word; \
        uint8_t bytes[2]; \
    } test = { .word = 0x0001 }; \
    test.bytes[0]; \
})

int main(int argc, char *argv[])
{
    printf("Endianness: %d (1 = Little Endian, 0 = Big Endian)\n", IS_LITTLE_ENDIAN);
    return 0;
}
