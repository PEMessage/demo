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

// ==============================================

typedef struct {
    SemaphoreHandle_t data_ready;  // Receiver waits on this
    SemaphoreHandle_t data_taken;  // Sender waits on this
    void* data;                    // Shared data pointer
} Channel;

// Initialize channel
Channel* channel_create(void) {
    Channel* ch = pvPortMalloc(sizeof(Channel));
    if (ch == NULL) return NULL;

    ch->data_ready = xSemaphoreCreateBinary();
    ch->data_taken = xSemaphoreCreateBinary();
    ch->data = NULL;

    return ch;
}

// Send data (blocks until receiver takes it)
BaseType_t channel_send(Channel* ch, void* data, TickType_t timeout) {
    if (ch == NULL) return pdFALSE;

    // Store data
    ch->data = data;

    // Signal receiver that data is ready
    xSemaphoreGive(ch->data_ready);

    // Wait for any previous data to be taken
    if (xSemaphoreTake(ch->data_taken, timeout) != pdTRUE) {
        return pdFALSE;  // Timeout
    }


    return pdTRUE;
}

// Receive data (blocks until sender sends)
BaseType_t channel_recv(Channel* ch, void** data_ptr, TickType_t timeout) {
    if (ch == NULL || data_ptr == NULL) return pdFALSE;

    // Wait for data to be ready
    if (xSemaphoreTake(ch->data_ready, timeout) != pdTRUE) {
        return pdFALSE;  // Timeout
    }

    // Get data
    *data_ptr = ch->data;
    ch->data = NULL;

    // Signal sender that data has been taken
    xSemaphoreGive(ch->data_taken);

    return pdTRUE;
}

// Try to receive without blocking
BaseType_t channel_try_recv(Channel* ch, void** data_ptr) {
    return channel_recv(ch, data_ptr, 0);
}

// Try to send without blocking
BaseType_t channel_try_send(Channel* ch, void* data) {
    return channel_send(ch, data, 0);
}

// Destroy channel
void channel_destroy(Channel* ch) {
    if (ch) {
        vSemaphoreDelete(ch->data_ready);
        vSemaphoreDelete(ch->data_taken);
        vPortFree(ch);
    }
}

// ==============================================
Channel *ch;
Channel *end;

#define ARRAY_INDEX(ptr, array) ((size_t)((void *)(ptr) - (void *)(array)) / sizeof((array)[0]))

int TASK_ARG[] = {
    300, // TaskSend0
    400, // TaskSend1
    100, // TaskRecv
};

void TaskSend(void *arg) {
    int delay = *(int*)arg;
    int id = ARRAY_INDEX(arg, TASK_ARG);

    while(1) {

        vTaskDelay(pdMS_TO_TICKS(delay));
        printf("%d: Send Start\n", id);
        channel_send(ch, NULL, portMAX_DELAY);
        printf("%d: Send End\n", id);

        channel_send(end, NULL, portMAX_DELAY);
        printf("%d: Send Final\n", id);

        while(1);

    }
}

void TaskRecv(void *arg) {
    int delay = *(int*)arg;
    int id = ARRAY_INDEX(arg, TASK_ARG);

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(delay));

        for (int i = 1; i < 2 ; i ++) {
            printf("%d: Recv Start %d\n", id, i);
            void *data;
            channel_recv(ch, &data, portMAX_DELAY);
            printf("%d: Recv End %d\n", id, i);
        }

        channel_send(end, NULL, portMAX_DELAY);
        printf("%d: Recv Final\n", id);

        while(1);
    }
}

void TaskMain() {
    ch = channel_create();
    end = channel_create();

    xTaskCreate( TaskSend,
            "TaskSend0",
            configMINIMAL_STACK_SIZE + 3*1024,
            &TASK_ARG[0],
            (tskIDLE_PRIORITY + 1),
            NULL );

    xTaskCreate( TaskSend,
            "TaskSend1",
            configMINIMAL_STACK_SIZE + 3*1024,
            &TASK_ARG[1],
            (tskIDLE_PRIORITY + 1),
            NULL );
    xTaskCreate( TaskRecv,
            "TaskRecv",
            configMINIMAL_STACK_SIZE + 3*1024,
            &TASK_ARG[2],
            (tskIDLE_PRIORITY + 1),
            NULL );


    for (int i = 0; i < 2 ; i++) {
        void* data;

        channel_recv(end, &data, portMAX_DELAY);
    }
    printf("End !!\n");
    while(1);

}


int main() {
    setup();
    printf("Init complete\n");


    xTaskCreate( TaskMain,
            "TaskMain",
            configMINIMAL_STACK_SIZE + 3*1024,
            NULL,
            (tskIDLE_PRIORITY + 1),
            NULL );

    vTaskStartScheduler();
    while(1);
}
