#include <stdint.h>
#include <stdio.h>
#include "CMSDK_CM3.h"

// Also See:
//  https://freertos.org/Documentation/02-Kernel/03-Supported-devices/04-Demos/Others/Debugging-Hard-Faults-On-Cortex-M-Microcontrollers
//  https://www.freertos.org/FreeRTOS_Support_Forum_Archive/July_2017/freertos_How_to_catch_code_that_caused_the_hard_fault_f16902b9j.html

/* The prototype shows it is a naked function - in effect this is just an
   assembly function. */

void HardFault_Handler( void ) __attribute__( ( naked ) );
/* The fault handler implementation calls a function called
   prvGetRegistersFromStack(). */
void HardFault_Handler(void) 
{
    __asm volatile
    (
        " tst lr, #4                                                \n" // 测试链接寄存器（LR）的第 2 位（#4 的二进制是 100）
                                                                        // 当发生异常时，LR 的值会被自动设置为 EXC_RETURN。
                                                                        //
        " ite eq                                                    \n" // 条件执行指令的开始。
                                                                        // 表示 "If-Then-Else" 结构，后续两条指令会根据条件分别执行。
                                                                        //
        " mrseq r0, msp                                             \n" //     将 MSP（主栈指针）的值加载到寄存器 r0 中。
                                                                        //     Move  Register from(<-) Special register
        " mrsne r0, psp                                             \n" //
        " ldr r1, [r0, #24]                                         \n" // 从 SP + 24 的地址处加载一个值到 r1。
                                                                        // [r0] [r1] [r2] [r3] [r12] [LR] [PC] [xPSR]
                                                                        //  ^                              ^
                                                                        //  SP                          r1 = *(SP+24) = PC
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " handler2_address_const: .word prvGetRegistersFromStack    \n"
    );
}


void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
/* These are volatile to try and prevent the compiler/linker optimising them
   away as the variables never actually get used. If the debugger won't show the
   values of the variables, make them global my moving their declaration outside
   of this function. */
    volatile uint32_t r0;
    volatile uint32_t r1;
    volatile uint32_t r2;
    volatile uint32_t r3;
    volatile uint32_t r12;
    volatile uint32_t lr; /* Link register. */
    volatile uint32_t pc; /* Program counter. */
    volatile uint32_t psr;/* Program status register. */

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];



    // Print register contents
    printf("\n\n[Hard Fault - StackFrame Register State (Hex)]\n");
    printf("R0  = 0x%08lX\n", r0);
    printf("R1  = 0x%08lX\n", r1);
    printf("R2  = 0x%08lX\n", r2);
    printf("R3  = 0x%08lX\n", r3);
    printf("R12 = 0x%08lX\n", r12);
    printf("LR  = 0x%08lX  // Return address\n", lr);
    printf("PC  = 0x%08lX  // Program counter (faulting instruction)\n", pc);
    printf("PSR = 0x%08lX\n", psr); // Includes N, Z, C, V flags + IPSR/EXC number

    // Optional: Print related system registers for detailed debug
    printf("\n[Hard Fault - System Registers(SCB)]\n");

    // HardFault Status Register (HFSR)
    // Indicates source of a hard fault.
    // Bit 30 (FORCED): Set if fault escalated from a configurable fault.
    //                  if 0x40000000, mean you need find why in CFSR
    // See:
    //  https://developer.arm.com/documentation/dui0552/a/cortex-m3-peripherals/system-control-block/hardfault-status-register
    printf("HFSR = 0x%08lX  // HardFault Status Register\n",
            SCB->HFSR);

    // Configurable Fault Status Register (CFSR)
    // Combines UsageFault, BusFault, and MemManage status bits.
    // Common bits:
    // - [0:7] (UFSR): Usage Fault Status
    // - [8:15] (BFSR): Bus Fault Status
    // - [16:23] (MMFSR): Memory Management Fault Status
    printf("CFSR = 0x%08lX  // Configurable Fault Status Register\n",
            SCB->CFSR);

    // Bus Fault Address Register (BFAR)
    // Holds the data bus fault address if BFARVALID in CFSR is set.
    printf("BFAR = 0x%08lX  // Bus Fault Address Register\n",
            SCB->BFAR);

    // Debug Fault Status Register (DFSR)
    // Shows if a debug event caused the exception.
    // Bit 0 (HALTED): Core halted due to debugger request.
    printf("DFSR = 0x%08lX  // Debug Fault Status Register\n",
            SCB->DFSR);

    // Auxiliary Fault Status Register (AFSR)
    // Vendor-specific additional fault information.
    printf("AFSR = 0x%08lX  // Auxiliary Fault Status Register\n",
            SCB->AFSR);

    /* When the following line is hit, the variables contain the register values. */
    for( ;; );
}

