#include <assert.h>
#include <stdio.h>

// Come from: CMake is cooked... 
// https://www.youtube.com/watch?v=NjgXMc4fynk // 43:07
#define shift(xs, xs_sz) (assert(xs_sz > 0), --xs_sz, *xs++)


int main(int argc, char **argv)
{
    while (argc > 0) {
        printf("%s\n", shift(argv, argc));
    }

    return 0;
}
