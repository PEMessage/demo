#include <FreeRTOSConfig.h>
#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

void NVIC_Init() {
    size_t i = 0;
    for (i = 0; i < 32; i++) {
        NVIC_SetPriority((IRQn_Type)i, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    }
}

void setup() {
    extern int stdout_init(void);
    stdout_init();
    NVIC_Init();
}


// Due to xTimer API is cocurrent safe, they use a queue to store all API call
// if queue is full, and we call xTimer API in 'Timers Service Task'
// it will deadlock 'Timers Service Task'
void deadlock_timer_callback(TimerHandle_t xTimer) {
    int call_count = 0;

    printf("Timer callback count: %d\n", call_count);

    // This will cause deadlock - stopping the timer from its own callback
    // The timer task tries to send a command to itself, blocking indefinitely
    for (call_count = 0 ; call_count < configTIMER_QUEUE_LENGTH ; call_count++) {
        /* printf("Attempting to stop timer from its own callback...\n"); */
        BaseType_t result = xTimerStop(xTimer, 0xFFFF);

        printf("xTimerStop count: %d\n", call_count);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

void monitoring_task(void *pvParameters) {
    int count = 0;
    while (1) {
        printf("|| Monitoring task running - count: %d\n", count++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main() {
    setup();
    printf("Init complete\n");
    printf("SystemCoreClock is %u\n", SystemCoreClock);

    // Create monitoring task to show when system becomes unresponsive
    xTaskCreate(monitoring_task,
        "Monitor",
        configMINIMAL_STACK_SIZE * 3,
        NULL,
        31,
        NULL
    );

    // Create the deadlock timer
    TimerHandle_t deadlock_timer = xTimerCreate(
        "DeadlockTimer",
        pdMS_TO_TICKS(500),        // 500ms period
        pdTRUE,                    // Auto-reload
        (void *)0,
        deadlock_timer_callback    // This callback will cause the deadlock
    );

    // Start the timer
    if (xTimerStart(deadlock_timer, 0) != pdPASS) {
        printf("Timer start failed!\n");
        return 1;
    }

    printf("Timer started. Deadlock will occur after a few callbacks...\n");
    printf("The monitoring task will stop printing when deadlock occurs\n");

    vTaskStartScheduler();

    // Should never reach here
    while (1);
    return 0;
}
