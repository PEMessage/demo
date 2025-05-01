#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// Come from: CMake is cooked... 
// https://www.youtube.com/watch?v=NjgXMc4fynk // 43:07
#define shift(xs_sz, xs) (assert((xs_sz) > 0), --(xs_sz), *(xs)++)

// couldn't be disable by NDEBUBG macro
// Hack: *(xs) is for pass compiler test, (2 branch of ? expr must be same type)
#define shift_v2(xs_sz, xs) ( \
        (xs_sz) > 0 ? \
        --(xs_sz), *(xs)++ \
        : \
        (fprintf(stderr, "%s:%d OUT OF BOUND\n",__FILE__,__LINE__), abort(), *(xs)) \
        )


int shift_demo(int argc, char **argv) {
    while (argc > 0) {
        printf("%s\n", shift(argc, argv));
    }
    return 0;
}

int shift_demo_v2(int argc, char **argv) {
    while (argc > 0) {
        printf("%s\n", shift_v2(argc, argv));
    }
    return 0;
}

int main(int argc, char **argv)
{
    printf("shift v1\n");
    shift_demo(argc, argv);
    printf("shift v2 (not recommand)\n");
    shift_demo_v2(argc, argv);

    return 0;
}
