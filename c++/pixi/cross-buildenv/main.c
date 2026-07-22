#include <stdio.h>

void print_libc_version(void) {
    #if defined(__UCLIBC__)
        printf("uClibc version: %d.%d.%d\n",
               __UCLIBC_MAJOR__, __UCLIBC_MINOR__, __UCLIBC_SUBLEVEL__);
    #elif defined(__GLIBC__)
        printf("glibc version (compile-time): %d.%d\n",
               __GLIBC__, __GLIBC_MINOR__);
        #ifdef __has_include
        #if __has_include(<gnu/libc-version.h>)
        #include <gnu/libc-version.h>
        printf("glibc version (runtime): %s\n", gnu_get_libc_version());
        #endif
        #endif
    #elif defined(_MSC_VER)
        printf("Microsoft C/C++ Compiler version: %d\n", _MSC_VER);
    #else
        printf("Unknown libc (possibly musl or other)\n");
    #endif
}

int main() {
    print_libc_version();
    return 0;
}
