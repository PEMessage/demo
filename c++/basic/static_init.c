#include <stdio.h>

int func(int index) {
    printf("This is %s: %d\n", __func__, index);

    return index;
}

static int a = func(0); 
static int b = func(1); 
// gcc: error
// Initializer element is not a compile-time constant
//
// g++:
//  success, you could run something before main, no more __attribute__((constructor))?
// This is func: 0
// This is func: 1
// This is main

int main(int argc, char *argv[])
{
    printf("This is %s\n", __func__);
    return 0;
}
