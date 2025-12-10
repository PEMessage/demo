#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CMSDK_CM3.h" // for SystemCoreClock
#include "FreeRTOS.h"
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
};

void ServiceTask() {
    while(1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        printf("%s: notify!\n", __func__);
        {
            xSemaphoreTake(resource_mutex, portMAX_DELAY);
            for (int i = 0; i < 1000000; i ++) {
                __NOP(); // some heavy work
            }
            xSemaphoreGive(resource_mutex);
        }
        printf("%s: done!\n", __func__);

    }
}

void ServiceTimerCallback(TimerHandle_t xTimer) {
    struct Service* s = (struct Service *)pvTimerGetTimerID(xTimer);
    xTaskNotifyGive(s->task);

}

void ServiceDelete(struct Service* s) {
    // delete timer
    if(s->timer) {
        xTimerDelete(s->timer, portMAX_DELAY);
        s->timer = NULL;
    }

    // delete task
    if(s->task) {
        vTaskDelete(s->task);
        s->task = NULL;
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

    // task
    if (xTaskCreate(ServiceTask,
                "SrvTsk",
                configMINIMAL_STACK_SIZE+256,
                s,
                PRIORTY,
                &s->task
                ) != pdTRUE) {
        printf("[E]: task create fail\n");
        ServiceDelete(s);
        return NULL;
    }

    // timer
    s->timer = xTimerCreate(
        "SrvTim",
        pdMS_TO_TICKS(PERIOD),
        pdTRUE,               // Auto-reload
        s,                 // Timer ID
        ServiceTimerCallback
    );
    if(!s->timer) {
        printf("[E]: timer create fail\n");
        ServiceDelete(s);
        return NULL;
    }

    // enable timer at the end
    xTimerStart(s->timer, portMAX_DELAY);
    return s;
}

void ServiceNotify(struct Service *s) {
    if(!s) { return; }

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
        if (xSemaphoreTake(resource_mutex, 0) != pdTRUE) {
            printf("%s: resource_mutex not clean!!\n", __func__);
            return; // return from task to cause kernel pannic
        } else {
            xSemaphoreGive(resource_mutex);
        }
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
