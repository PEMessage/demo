// q-gcc: -finstrument-functions --
#include <stdio.h>

extern "C" { // We must have this block for using g++
    void __cyg_profile_func_enter(void *this_fn, void *call_site);
    void __cyg_profile_func_exit(void *this_fn, void *call_site);
}

int INDENT = 0;

void __attribute__((no_instrument_function)) // avoid recusive track itself
__cyg_profile_func_enter(void *this_fn, void *call_site) {
    {
        printf("T|");
        for ( int i = 0; i < INDENT; i++) {
            printf("  ");
        }
        printf("%p {\n", call_site);
    }
    INDENT++;
}

void __attribute__((no_instrument_function)) // avoid recusive track itself
__cyg_profile_func_exit(void *this_fn, void *call_site) {
    INDENT--;
    {
        printf("T|");
        for ( int i = 0; i < INDENT; i++) {
            printf("  ");
        }
        printf("} // %p\n", call_site);
    }
}

void foo3() {
    printf("This is %s\n", __func__);
    return;
}

void foo2() {
    printf("This is %s\n", __func__);
    foo3();
    return;
}

void foo1() {
    printf("This is %s\n", __func__);
    foo2();
    return;
}


int main(int argc, char *argv[])
{
    foo1();
    return 0;
}
