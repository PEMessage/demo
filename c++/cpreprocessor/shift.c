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

#include <string.h>
int parse(int argc, char **argv) {
    const char *program_name = shift(argc, argv);

    if (argc > 0) {
        const char *command_name = shift(argc, argv);
        if (strcmp(command_name, "version") == 0) {
            printf("%s Version is 1.0.0\n", program_name);
            return 0;
        } else {
            fprintf(stderr, "Usage: %s [version]\n", program_name);
            fprintf(stderr, "ERROR: unknown command %s\n", command_name);
            return 1;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    printf("[Info]: shift v1\n");
    shift_demo(argc, argv);
    printf("[Info]: shift v2 (not recommand)\n");
    shift_demo_v2(argc, argv);
    printf("[Info]: demo cmdline parse \n");
    parse(argc, argv);

    return 0;
}
