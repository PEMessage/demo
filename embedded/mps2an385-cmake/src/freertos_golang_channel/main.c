#include <stdio.h>
#include <assert.h>

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
    QueueHandle_t queue;
} Channel;

// Initialize channel
Channel* channel_create(void) {
    Channel* ch = pvPortMalloc(sizeof(Channel));
    if (ch == NULL) return NULL;

    ch->queue = xQueueCreate(1, sizeof(void*));

    return ch;
}

const int HEADER = 0xFFAE;

// Send data (blocks until receiver takes it)
BaseType_t channel_send(Channel* ch, void* data, TickType_t timeout) {
    if (ch == NULL) return pdFALSE;

    xQueueSend(ch->queue, &HEADER, portMAX_DELAY);
    return xQueueSend(ch->queue, &data, timeout);
}

// Receive data (blocks until sender sends)
BaseType_t channel_recv(Channel* ch, void** data_ptr, TickType_t timeout) {
    if (ch == NULL || data_ptr == NULL) return pdFALSE;

    int header;
    xQueueReceive(ch->queue, &header , portMAX_DELAY);
    // assert(header == HEADER);
    return xQueueReceive(ch->queue, data_ptr, timeout);
}

// Destroy channel
void channel_destroy(Channel* ch) {
    // if (ch) {
    //     vSemaphoreDelete(ch->data_ready);
    //     vSemaphoreDelete(ch->data_taken);
    //     vPortFree(ch);
    // }
}

// ==============================================
Channel *ch;
Channel *end;

#define ARRAY_INDEX(ptr, array) ((size_t)((void *)(ptr) - (void *)(array)) / sizeof((array)[0]))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

int TASK_ARG[] = {
    300, // TaskSend0
    301, // TaskSend1
    500, // TaskRecv
};

void TaskSend(void *arg) {
    int delay = *(int*)arg;
    int id = ARRAY_INDEX(arg, TASK_ARG);

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(delay));

        printf("%d: Send Start\n", id);
        channel_send(ch, (void *)id, portMAX_DELAY);
        printf("%d: Send End\n", id);

        channel_send(end, (void *)id, portMAX_DELAY);
        printf("%d: Send Final\n", id);

        // if task have high prio,
        // `while(1)` or `while(1) { portYIELD(); }`
        // will cause any task lowwer then it unable to schedule
        // we should `vTaskDelete(NULL)` this or `while(1) { vTaskDelay(SOMETIME); }`
        vTaskDelete(NULL);

    }
}

void TaskRecv(void *arg) {
    int delay = *(int*)arg;
    int id = ARRAY_INDEX(arg, TASK_ARG);

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(delay));

        for (int i = 0; i < 2 ; i ++) {
            printf("%d: Recv Start %d\n", id, i);
            void *data;
            channel_recv(ch, &data, portMAX_DELAY);

            int recv = (int)data;
            printf("%d: Recv End %d -- %d\n", id, i, recv);
        }

        channel_send(end, (void *)id, portMAX_DELAY);
        printf("%d: Recv Final\n", id);

        vTaskDelete(NULL);
    }
}

void TaskMain() {
    ch = channel_create();
    end = channel_create();

    while(1) {
        vTaskDelay(10);
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


        int data[ARRAY_SIZE(TASK_ARG)] = {0};
        for (int i = 0; i < ARRAY_SIZE(TASK_ARG) ; i++) {
            channel_recv(end, &data[i], portMAX_DELAY);
            printf("End %d, data -- %d\n",i, data[i]);
        }
        printf("End !!\n");
    }
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
