#include <stdio.h>
extern int stdout_init (void);

int main() {
    stdout_init();
    while(1){
        // ZJW_C stdout_putchar('a');
        printf("Hello world\n");
    }
}
