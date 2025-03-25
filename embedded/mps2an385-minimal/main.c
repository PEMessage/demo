#include <stdio.h>
extern int stdout_init (void);

int main() {
    stdout_init();
    while(1){
        stdout_putchar('a');
    }
}
