#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "task.h"

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

void TaskPing() {
    while(1) {
        printf("Ping!\n");
        vTaskDelay(300);
    }
}


unsigned int *base = NULL;
void recusive_print() {
    unsigned int a = 0;
    vTaskDelay(80);
    printf("Pointer address is 0x%08x, Size is %d\n", (unsigned int)&a, base - &a);
    recusive_print();
}

void TaskOverFlow() {
    vTaskDelay(3000);
    unsigned int a = 0;
    base = &a;
    recusive_print();
}

int main() {
    setup();
    printf("Init complete\n");
    xTaskCreate( TaskPing,
            "TaskPing",
            configMINIMAL_STACK_SIZE + 3*1024,
            NULL,
            (tskIDLE_PRIORITY + 1),
            NULL );
    xTaskCreate( TaskOverFlow,
            "TaskOverFlow",
            configMINIMAL_STACK_SIZE + 512,
            NULL,
            (tskIDLE_PRIORITY + 1),
            NULL );
    vTaskStartScheduler();
    while(1);
}
