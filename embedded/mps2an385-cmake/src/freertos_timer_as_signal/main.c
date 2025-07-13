#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"


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

void print_current() {
    TickType_t current_time = xTaskGetTickCount();
    printf("Current time: %u ms\n",
            current_time * portTICK_PERIOD_MS);
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    printf("Current Task: %s\n", pcTaskGetName(current_task));
}


TimerHandle_t auto_reload_timer = 0;
// Timer callback function
void timer_callback(TimerHandle_t xTimer) {
    print_current();
    xTimerStop(auto_reload_timer, portMAX_DELAY);
}

int main() {
    setup();
    printf("Init complete\n");
    printf("SystemCoreClock is %u\n", SystemCoreClock);

    auto_reload_timer = xTimerCreate(
            "AutoReloadTimer",          // Timer name (for debugging)
            pdMS_TO_TICKS(1000),       // Timer period in ticks (1000ms = 1s)
            pdTRUE,                    // Auto-reload (pdTRUE for auto-repeat)
            (void *)0,                 // Timer ID (can be used to identify timer)
            timer_callback             // Callback function
            );

    // Check if timer was created successfully
    if (auto_reload_timer == NULL) {
        printf("Timer creation failed!\n");
        return 1;
    }

    // Start the timer
    if (xTimerStart(auto_reload_timer, 0) != pdPASS) {
        printf("Timer start failed!\n");
        return 1;
    }

    // Create the timer reset task
    void timer_reset_task(void *pvParameters);
    xTaskCreate(
        timer_reset_task,          // Task function
        "TimerResetTask",         // Task name
        configMINIMAL_STACK_SIZE, // Stack size
        NULL,                     // Parameters
        tskIDLE_PRIORITY + 1,    // Priority
        NULL                      // Task handle
    );


    vTaskStartScheduler();
    while(1);
}


// Task that periodically resets the timer
void timer_reset_task(void *pvParameters) {
    const TickType_t reset_period = pdMS_TO_TICKS(2500); // Reset every 2.5 seconds

    while(1) {
        vTaskDelay(reset_period);

        printf("\n--- Resetting timer ---\n");
        // using reset as a signal to triggle timer_callback
        if(xTimerReset(auto_reload_timer, portMAX_DELAY) != pdPASS) {
            printf("Timer reset failed!\n");
        } else {
            printf("Timer reset success!\n");
        }
    }
}

