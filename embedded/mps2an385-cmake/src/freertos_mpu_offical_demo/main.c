#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"


void NVIC_Init() {
    size_t i = 0 ;
    for ( i = 0; i < 32; i++) {
        // NVIC_DisableIRQ(i);
        NVIC_SetPriority((IRQn_Type)i, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    }
}


void setup() {
    extern int stdout_init (void);
    stdout_init();
    NVIC_Init();
}


/* Demo includes. */
#include "mpu_demo.h"

int main() {
    setup();
    printf("Init complete\n");

    vStartMPUDemo();

    vTaskStartScheduler();
    while(1);
}


