#include <stdio.h>
#include <stdlib.h>
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
// ===========================================

struct Service {
    TaskHandle_t task;
    TimerHandle_t timer;
};

void ServiceTask() {
    while(1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        printf("%s: notify!\n", __func__);
    }
}

void ServiceTimerCallback(TimerHandle_t xTimer) {
    struct Service* s = (struct Service *)pvTimerGetTimerID(xTimer);
    xTaskNotifyGive(s->task);

}

void ServiceDelete(struct Service* s) {
    if(s->timer) {
        xTimerDelete(s->timer, portMAX_DELAY);
        s->timer = NULL;
    }
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

    if (xTaskCreate(ServiceTask,
                "SrvTsk",
                configMINIMAL_STACK_SIZE+256,
                s,
                10,
                &s->task
                ) != pdTRUE) {
        printf("[E]: task create fail\n");
        ServiceDelete(s);
        return NULL;
    }

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

    xTimerStart(s->timer, portMAX_DELAY);
    return s;
}

void ServiceNotify(struct Service *s) {
    if(!s) { return; }

    xTaskNotifyGive(s->task);
}




// ===========================================
void MainTask() {
    printf("%s: start\n", __func__);
    struct Service* s = ServiceCreate();
    printf("%s: create done\n", __func__);

    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("%s: notify from external\n", __func__);
        ServiceNotify(s);

        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("%s: notify from external\n", __func__);
        ServiceNotify(s);

        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("%s: notify from external\n", __func__);
        ServiceNotify(s);
    }

    ServiceDelete(s);
    printf("%s: End\n", __func__);
    while(1);

}

int main() {
    setup();
    printf("Init complete\n");
    xTaskCreate( MainTask,
            "MainTask",
            configMINIMAL_STACK_SIZE + 3*1024,
            NULL,
            (tskIDLE_PRIORITY + 1),
            NULL );
    vTaskStartScheduler();
    while(1);
}
