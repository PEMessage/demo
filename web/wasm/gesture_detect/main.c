#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define EXPORT(name) __attribute__((export_name(name)))


int main(int argc, char *argv[]) {
    printf("hello, world!\n");

    for (int i = 0 ; i < argc ; i++) {
        printf("argv[%d]: %s\n", i, argv[i]);
    }

    return 0;

}

EXPORT("pollonce") void pollonce(int x, int y, bool pressed, uint16_t tick) {
    printf("x %d, y %d, pressed %d, tick %d\n", x, y, pressed, tick);
}
