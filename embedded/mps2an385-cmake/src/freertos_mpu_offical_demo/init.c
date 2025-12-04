#include <stdio.h>

// static void MemMang_Handler( void ) __attribute__( ( naked ) );

void MemMang_Handler( void )
{
    __asm volatile
    (
        " tst lr, #4                                                         \n"
        " ite eq                                                             \n"
        " mrseq r0, msp                                                      \n"
        " mrsne r0, psp                                                      \n"
        " ldr r1, =vHandleMemoryFault                                        \n"
        " bx r1                                                              \n"
        " .ltorg                                                             \n"
    );
}

void BusFault_Handler( void )
{
    __asm volatile
    (
        " tst lr, #4                                                         \n"
        " ite eq                                                             \n"
        " mrseq r0, msp                                                      \n"
        " mrsne r0, psp                                                      \n"
        " ldr r1, =vHandleMemoryFault                                        \n"
        " bx r1                                                              \n"
        " .ltorg                                                             \n"
    );
}

void UsageFault_Handler( void )
{
    __asm volatile
    (
        " tst lr, #4                                                         \n"
        " ite eq                                                             \n"
        " mrseq r0, msp                                                      \n"
        " mrsne r0, psp                                                      \n"
        " ldr r1, =vHandleMemoryFault                                        \n"
        " bx r1                                                              \n"
        " .ltorg                                                             \n"
    );
}

void Debug_Handler( void )
{
    __asm volatile
    (
        " tst lr, #4                                                         \n"
        " ite eq                                                             \n"
        " mrseq r0, msp                                                      \n"
        " mrsne r0, psp                                                      \n"
        " ldr r1, =vHandleMemoryFault                                        \n"
        " bx r1                                                              \n"
        " .ltorg                                                             \n"
    );
}
