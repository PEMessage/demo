#include <execinfo.h>  // for backtrace
#include <stdio.h>     // for printf
#include <stdlib.h>    // for free

void print_backtrace() {
    void *buffer[10];  // Array to store backtrace addresses
    int size = backtrace(buffer, 10);  // Capture up to 10 stack frames

    // Get human-readable symbols (function names)
    char **symbols = backtrace_symbols(buffer, size);

    if (symbols == NULL) {
        perror("backtrace_symbols");
        return;
    }

    printf("Backtrace (most recent call first):\n");
    for (int i = 0; i < size; i++) {
        printf("%2d: %s\n", i, symbols[i]);
    }

    free(symbols);  // Must free the array allocated by backtrace_symbols
}

// A dummy function to demonstrate the backtrace
void foo() {
    print_backtrace();
}

int main() {
    foo();
    return 0;
}
