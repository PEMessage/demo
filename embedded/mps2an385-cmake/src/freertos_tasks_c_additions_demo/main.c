#include <stdio.h>
#include <stdint.h>
#include "CMSDK_CM3.h"
#include "FreeRTOS.h"
#include "task.h"
#include "freertos_tasks_c_additions_include.h"

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

void TaskTest(void *pvParameters) {
    intptr_t delay = (intptr_t)pvParameters;
    vTaskDelay(delay);

    TaskHandle_t handle = xTaskGetCurrentTaskHandle();

    printf("%s:\n", pcTaskGetName(handle));
    printf("  stack size words: %lu\n", (unsigned long)taskGetStackSizeWords(handle));
    printf("  stack size bytes: %lu\n", (unsigned long)taskGetStackSizeBytes(handle));
    printf("\n");

    while (1) {
        vTaskDelay(1000);
    }
}

int main() {
    TaskHandle_t h1 = NULL, h2 = NULL;

    setup();
    printf("=== taskGetStackSizeWords / taskGetStackSizeBytes demo ===\n\n");

    xTaskCreate(TaskTest, "Task_512w",
            configMINIMAL_STACK_SIZE + 512,
            (void *)10, (tskIDLE_PRIORITY + 1), &h1);

    xTaskCreate(TaskTest, "Task_1024w",
            configMINIMAL_STACK_SIZE + 1024,
            (void *)15, (tskIDLE_PRIORITY + 1), &h2);

    // printf("TaskHandle h1=%p, h2=%p\n", (void *)h1, (void *)h2);
    // printf("h1 stack words: %lu\n", (unsigned long)taskGetStackSizeWords(h1));
    // printf("h1 stack bytes: %lu\n", (unsigned long)taskGetStackSizeBytes(h1));
    // printf("\n");

    vTaskStartScheduler();
    while (1);
}
