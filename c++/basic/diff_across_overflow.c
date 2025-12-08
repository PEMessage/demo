#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>

// For FreeRTOS or embeding system, we are almost safe to diff two tick, without consider overflow unless diff in following range
// reference: UINT16_MAX(65535) * 1ms == 65.535 s ~= 1min
// reference: UINT32_MAX(4294967295U) * 1ms == 4,294,967 s == 1193 hour == 49 days
int main() {
    {
        uint8_t a =  UINT8_MAX;
        uint8_t diff = UINT8_MAX - 1;
        uint8_t b = a + diff;
        uint8_t result = b - a;
        printf("b(%d) - a(%d) = result(%d) vs diff(%d)\n", b, a, result, diff);
        assert(result == diff);
    }
    {
        uint8_t a =  UINT8_MAX;
        uint8_t diff = UINT8_MAX;
        uint8_t b = a + diff;
        uint8_t result = b - a;
        printf("b(%d) - a(%d) = result(%d) vs diff(%d)\n", b, a, result, diff);
        assert(result == diff);
    }
    {
        uint8_t a =  UINT8_MAX;
        uint8_t diff = UINT8_MAX + 1;
        uint8_t b = a + diff;
        uint8_t result = b - a;
        printf("b(%d) - a(%d) = result(%d) vs diff(%d)\n", b, a, result, diff);
        assert(result == diff);
    }
}
