#include <stdio.h>
#include <stdint.h>
#include "cmsis_gcc.h" // for __get_CONTROL
#include "CMSDK_CM3.h" // for __get_CONTROL
extern int stdout_init (void);


void trigger_hardfault(void) {
    __asm volatile ("bkpt #0"); // Undefined instruction
}

int main() {

    __enable_fault_irq();
    SystemCoreClockUpdate();
    stdout_init();
    // prvUARTInit();

    trigger_hardfault();

    while(1) {
        // printf("Now we are in SVC_Handler_Main\n");
    }

}
