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

void TaskPong() {
    while(1) {
        printf("Pong!\n");
        vTaskDelay(40);
    }

}

int main() {
    setup();
    xTraceInitialize();
    xTraceEnable(TRC_START);
    xTraceTimestampSetPeriod(configCPU_CLOCK_HZ/configTICK_RATE_HZ);

    printf("Init complete\n");
    xTaskCreate( TaskPing,
            "TaskPing",
            configMINIMAL_STACK_SIZE + 3*1024,
            NULL,
            (tskIDLE_PRIORITY + 1),
            NULL );
    xTaskCreate( TaskPong,
            "TaskPong",
            configMINIMAL_STACK_SIZE + 3*1024,
            NULL,
            (tskIDLE_PRIORITY + 1),
            NULL );
    vTaskStartScheduler();
    while(1);
}
