#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

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


static SemaphoreHandle_t xMutexUnSafe = NULL;
static SemaphoreHandle_t xMutexSafe = NULL;
static TaskHandle_t xTaskToDelete = NULL;

enum TaskID {
    SAFE_TASK_ID,
    UNSAFE_TASK_ID,
    MONITOR_TASK_ID,
    TASK_NUM
};

static TaskHandle_t tasks[TASK_NUM] = {};

/* Safe task with cleanup */
void SafeTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1) {
        // Try to take mutex with timeout
        if(xSemaphoreTake(xMutexSafe, pdMS_TO_TICKS(portMAX_DELAY)) == pdTRUE) {
            printf("SafeTask: Mutex acquired\n");

            // Simulate work while holding mutex
            vTaskDelay(pdMS_TO_TICKS(500));

            // Check if we should terminate (clean exit point)
            if(xTaskToDelete == xTaskGetCurrentTaskHandle()) {
                printf("SafeTask: Releasing mutex before deletion\n");
                xSemaphoreGive(xMutexSafe);
                vTaskDelay(1);  // Allow scheduler to run
                printf("SafeTask: Now safe to delete\n");
                vTaskDelete(NULL);
            }

            xSemaphoreGive(xMutexSafe);
            printf("SafeTask: Mutex released\n");
        }

        // vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* Unsafe task that doesn't release mutex */
void UnsafeTask(void *pvParameters) {
    while(1) {
        if(xSemaphoreTake(xMutexUnSafe, portMAX_DELAY) == pdTRUE) {
            printf("UnsafeTask: Mutex acquired\n");

            // Hold mutex for a while
            vTaskDelay(pdMS_TO_TICKS(500));

            // Task could be deleted here while holding mutex - UNSAFE!
            if(xTaskToDelete == xTaskGetCurrentTaskHandle()) {
                printf("UnsafeTask: Being deleted while holding mutex!\n");
                vTaskDelay(10);
                // Mutex NOT released - THIS IS UNSAFE
                vTaskDelete(NULL);
            }

            xSemaphoreGive(xMutexUnSafe);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* Monitor task to demonstrate the problem */
void MonitorTask(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(300));

    printf("\n=== Test 1: Trying to delete UnsafeTask (BAD) ===\n");
    xTaskToDelete = tasks[UNSAFE_TASK_ID]; // Set for unsafe task

    // Try to take mutex from monitor task - will block if unsafe task holds it
    printf("Monitor: Trying to take mutex...\n");
    if(xSemaphoreTake(xMutexUnSafe, pdMS_TO_TICKS(2000)) == pdTRUE) {
        printf("Monitor: Got mutex (unexpected!)\n");
        xSemaphoreGive(xMutexUnSafe);
    } else {
        printf("Monitor: TIMEOUT - Mutex still locked by deleted task!\n");
        printf("Monitor: Resource leak and potential deadlock!\n");
    }

    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("\n=== Test 2: Trying to delete SafeTask (Good) ===\n");
    xTaskToDelete = tasks[SAFE_TASK_ID]; // Set for unsafe task
                                           //
    if(xSemaphoreTake(xMutexSafe, pdMS_TO_TICKS(2000)) == pdTRUE) {
        printf("Monitor: Got mutex (expected!)\n");
        xSemaphoreGive(xMutexSafe);
    } else {
        printf("Monitor: TIMEOUT - Mutex still locked by deleted task!\n");
        printf("Monitor: Resource leak and potential deadlock!\n");
    }
    printf("\n=== done ===\n");

    while(1) {
        printf("Monitor: System still running...\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


int main() {
    setup();
    printf("FreeRTOS Mutex Safety Demo\n");

    // Create mutex
    xMutexUnSafe = xSemaphoreCreateMutex();
    xMutexSafe = xSemaphoreCreateMutex();
    if(xMutexUnSafe == NULL || xMutexSafe == NULL) {
        printf("Failed to create mutex\n");
        while(1);
    }

    // Create tasks
    xTaskCreate(SafeTask, "SafeTask", configMINIMAL_STACK_SIZE + 1024,
                NULL, tskIDLE_PRIORITY + 2, &tasks[SAFE_TASK_ID]);

    xTaskCreate(UnsafeTask, "UnsafeTask", configMINIMAL_STACK_SIZE + 1024,
                NULL, tskIDLE_PRIORITY + 2, &tasks[UNSAFE_TASK_ID]);

    xTaskCreate(MonitorTask, "Monitor", configMINIMAL_STACK_SIZE + 1024,
                NULL, tskIDLE_PRIORITY + 3, &tasks[MONITOR_TASK_ID]);

    vTaskStartScheduler();

    while(1);
    return 0;
}
