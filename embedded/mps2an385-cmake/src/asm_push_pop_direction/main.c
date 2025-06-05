#include <stdio.h>
#include <stdint.h>
#include "cmsis_gcc.h" // for __get_CONTROL
#include "CMSDK_CM3.h" // for __get_CONTROL
extern int stdout_init (void);



void __attribute__ ((optimize("O0"))) push_pop() {
    register uint32_t val_r1 __asm__("r1") = 0x12345678;
    register uint32_t val_r2 __asm__("r2") = 0x87654321;

    uint32_t *sp_after_push = 0 ;
    // TODO create a variable to store sp
    // Use inline asm to push r4 and lr onto the stack
    __asm__ volatile (
            "push {r1, r2}      \n"   // Push both registers
            "mov %0, sp         \n"   // move current sp to var
            : "=r"(sp_after_push)
            :                   // No input operands
            : "memory"          // Memory clobber
            );


    uint32_t* stack_base = sp_after_push ;  // Points to where PUSH started

    uint32_t first = stack_base[0];  // First value pushed
    uint32_t second = stack_base[1]; // Second value pushed

    printf("Exec: push {r1, r2}\n");
    printf("Result:\n");
    printf("  stack_base[0] is 0x%04lx // r1 is lasted pushed\n", first);
    printf("  stack_base[1] is 0x%04lx // r2 is first pushed\n", second);
    printf("Diagram:\n");
    printf("        push {r1, r2}\n");
    printf("        <<<< -------- <<<< push\n");
    printf("    pop >>>> -------- >>>>     \n");

    while (1);
}

int main() {
    stdout_init();
    
    printf("Init complete\n");
    push_pop();

}
