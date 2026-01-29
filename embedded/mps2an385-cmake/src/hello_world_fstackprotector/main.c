#include <stdio.h>
#include <string.h>
extern int stdout_init (void);

extern uintptr_t __stack_chk_guard;
void vulnerable_function() {
    char buffer[10];

    printf("Buffer address: %p\n", buffer);
    printf("guard value is: %u \n", __stack_chk_guard);

    strcpy(buffer, "THIS----THIS\n");

    printf("%s", buffer);
}


int main() {
    stdout_init();
    printf("Testing fstack-protector...\n");
    char new[] = "This is something create on main";

    vulnerable_function();
    printf("If you see this, stack protection may be disabled\n");
    printf("%s\n", new);

    return 0;
}

// See: https://stackoverflow.com/a/42174846/20689546
void __wrap___stack_chk_fail(void);
void __real___stack_chk_fail(void);

void __wrap___stack_chk_fail(void)
{
    printf("==== Hit ====\n");
    __real___stack_chk_fail();
}
