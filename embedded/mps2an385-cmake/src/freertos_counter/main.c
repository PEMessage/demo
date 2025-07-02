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

void TaskHighSpeedPoll() {
    uint32_t cnt = 0;
    TickType_t previous_time = xTaskGetTickCount();
    TickType_t current_time;

    while(1) {
        cnt ++;
        if (cnt % 1000000 == 0) {
            current_time = xTaskGetTickCount();

            // Calculate time difference in milliseconds
            TickType_t time_diff = (current_time - previous_time) * portTICK_PERIOD_MS;

            printf("Current time: %u ms, Previous time: %u ms, Difference: %u ms\n",
                    current_time * portTICK_PERIOD_MS,
                    previous_time * portTICK_PERIOD_MS,
                    time_diff);

            // Save current time as previous for next iteration
            previous_time = current_time;
            cnt = 0;
        }

    }
}


int main() {
    setup();
    printf("Init complete\n");
    xTaskCreate( TaskHighSpeedPoll,
            "TaskHighSpeedPoll",
            configMINIMAL_STACK_SIZE + 3*1024,
            NULL,
            (tskIDLE_PRIORITY + 1),
            NULL );
    vTaskStartScheduler();
    while(1);
}
