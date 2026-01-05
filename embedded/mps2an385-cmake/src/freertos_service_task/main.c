#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "CMSDK_CM3.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "portmacro.h"
#include "task.h"
#include "timers.h"
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
// ===========================================

static SemaphoreHandle_t resource_mutex = NULL;

// NOTE:
//
// we must have MainTask and ServiceTask in same PRIORTY,
// and enable configUSE_PREEMPTION to enable taskswitch
// thus we could cause it assert
const int PRIORTY = 10;

struct Service {
    TaskHandle_t task;
    TimerHandle_t timer;
    volatile bool terminate_request;  // Flag to request termination
    SemaphoreHandle_t terminate_sem;  // Semaphore to signal deletion completion
};

void ServiceTask(void* param) {
    struct Service* s = (struct Service*)param;

    while(1) {
        // Wait for notification or termination request
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(portMAX_DELAY)); // Check periodically

        // Check if termination was requested
        if (s->terminate_request) {
            printf("%s: Termination requested, cleaning up...\n", __func__);

            // Stop the timer if it exists
            if(s->timer) {
                xTimerStop(s->timer, portMAX_DELAY);
            }

            // Signal that we're about to delete ourselves
            xSemaphoreGive(s->terminate_sem);

            // Delete ourselves
            vTaskDelete(NULL);
        }

        // Normal processing
        printf("%s: processing...\n", __func__);
        {
            xSemaphoreTake(resource_mutex, portMAX_DELAY);
            for (int j = 0; j < 2000; j ++) {
                for (int i = 0; i < 1000000; i ++) {
                    __NOP(); // some heavy work
                }
            }
            xSemaphoreGive(resource_mutex);
        }
        printf("%s: done!\n", __func__);

    }
}

void ServiceTimerCallback(TimerHandle_t xTimer) {
    struct Service* s = (struct Service *)pvTimerGetTimerID(xTimer);

    // Don't notify if termination was requested
    if (!s->terminate_request) {
        xTaskNotifyGive(s->task);
    }
}

void ServiceDelete(struct Service* s) {
    if (!s) return;

    // Request termination
    s->terminate_request = true;

    // Notify the task to wake it up
    xTaskNotifyGive(s->task);

    // Wait for the task to signal it's ready to be deleted
    if (xSemaphoreTake(s->terminate_sem, pdMS_TO_TICKS(portMAX_DELAY)) == pdTRUE) {
        printf("Service task is ready for deletion\n");

        // Now it's safe to delete the timer
        if(s->timer) {
            xTimerDelete(s->timer, portMAX_DELAY);
            s->timer = NULL;
        }

        // Task has already deleted itself, so we just need to clean up the handle
        s->task = NULL;

        // Clean up the semaphore
        vSemaphoreDelete(s->terminate_sem);
        s->terminate_sem = NULL;
    } else {
        printf("ERROR: Service task didn't respond to termination request\n");
    }
}

#define PERIOD 100
struct Service* ServiceCreate() {
    struct Service* s = malloc(sizeof(*s));
    if(!s) {
        printf("[E]: malloc fail\n");
        return NULL;
    }

    // init
    memset(s, 0, sizeof(*s));
    s->terminate_request = false;

    // Create termination semaphore
    s->terminate_sem = xSemaphoreCreateBinary();
    if (!s->terminate_sem) {
        printf("[E]: semaphore create fail\n");
        free(s);
        return NULL;
    }

    // Create task
    if (xTaskCreate(ServiceTask,
                "SrvTsk",
                configMINIMAL_STACK_SIZE+256,
                s,
                PRIORTY,
                &s->task
                ) != pdTRUE) {
        printf("[E]: task create fail\n");
        vSemaphoreDelete(s->terminate_sem);
        free(s);
        return NULL;
    }

    // Create timer
    s->timer = xTimerCreate(
        "SrvTim",
        pdMS_TO_TICKS(PERIOD),
        pdTRUE,               // Auto-reload
        s,                    // Timer ID
        ServiceTimerCallback
    );
    if(!s->timer) {
        printf("[E]: timer create fail\n");
        // Request task termination and wait
        s->terminate_request = true;
        xTaskNotifyGive(s->task);
        vTaskDelay(pdMS_TO_TICKS(10)); // Give task a chance to terminate
        vSemaphoreDelete(s->terminate_sem);
        free(s);
        return NULL;
    }

    // Enable timer
    xTimerStart(s->timer, portMAX_DELAY);
    return s;
}

void ServiceNotify(struct Service *s) {
    if(!s || s->terminate_request) { return; }

    xTaskNotifyGive(s->task);
}

// ===========================================
void MainTask() {
    int counter = 0;
    while(1) {
        printf("%s: start, counter: %d \n", __func__, counter++);
        struct Service* s = ServiceCreate();
        printf("%s: create done\n", __func__);

        {
            vTaskDelay(pdMS_TO_TICKS(200));
            printf("%s: notify from external\n", __func__);
            ServiceNotify(s);

            vTaskDelay(pdMS_TO_TICKS(200));
            printf("%s: notify from external\n", __func__);
            ServiceNotify(s);

            vTaskDelay(pdMS_TO_TICKS(200));
            printf("%s: notify from external\n", __func__);
            ServiceNotify(s);

            vTaskDelay(pdMS_TO_TICKS(200));
        }

        ServiceDelete(s);

        // Free the service structure
        free(s);
        s = NULL;

        // Verify mutex is clean
        if (xSemaphoreTake(resource_mutex, 0) != pdTRUE) {
            printf("%s: resource_mutex not clean!!\n", __func__);
            return; // return from task to cause kernel panic
        } else {
            xSemaphoreGive(resource_mutex);
        }

        printf("%s: Service deleted successfully\n", __func__);
    }

    printf("%s: End\n", __func__);
    while(1);
}

int main() {
    setup();
    printf("Init complete\n");
    resource_mutex = xSemaphoreCreateMutex();
    xTaskCreate( MainTask,
            "MainTask",
            configMINIMAL_STACK_SIZE + 3*1024,
            NULL,
            PRIORTY,
            NULL );
    vTaskStartScheduler();
    while(1);
}
