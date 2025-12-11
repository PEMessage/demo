#include <stdio.h>
#include <assert.h>

#include "CMSDK_CM3.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "portable.h"
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

void assert_unique(int arr[], int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = i + 1; j < size; j++) {
            assert(arr[i] != arr[j]);
        }
    }
}
// ==============================================

// A golang like unbuffer-channel
// Recvice is block operation(obivious)
// Send is also a block operation(not obivious, unbuffer prop)
//
// so we could do a commuication and synchronization in single operation
// which ease how to write a cocurrent program.
typedef struct {
    QueueHandle_t queue;
    SemaphoreHandle_t send_mutex;
    SemaphoreHandle_t recv_mutex;
} Channel;

// Initialize channel
Channel* channel_create(void) {
    Channel* ch = pvPortMalloc(sizeof(Channel));
    if (ch == NULL) return NULL;

    size_t free_heap_before = xPortGetFreeHeapSize();

    ch->queue = xQueueCreate(1, sizeof(void*));
    // NOTE: using mutex will cause ASSERT trigger
    // ASSERT! Line 6755, file FreeRTOS-Kernel/code/tasks.c
    // ch->send_mutex = xSemaphoreCreateMutex();
    // ch->recv_mutex = xSemaphoreCreateMutex();

    // But using xSemaphoreBinary will not
    // TODO: why?
    ch->send_mutex = xSemaphoreCreateBinary();
    ch->recv_mutex = xSemaphoreCreateBinary();
    xSemaphoreGive(ch->send_mutex);
    xSemaphoreGive(ch->recv_mutex);

    size_t free_heap_after = xPortGetFreeHeapSize();
    printf("Free Heap: %d bytes\n", free_heap_before - free_heap_after);
    return ch;
}

const int HEADER = 0xFFAE;

// Send data (blocks until receiver takes it)
BaseType_t channel_send(Channel* ch, void* data, TickType_t timeout) {
    if (ch == NULL) return pdFALSE;

    if (xSemaphoreTake(ch->send_mutex, timeout) != pdTRUE) {
        return pdFALSE;
    }
    BaseType_t result;

    // Since, we got send mutex, not body are sending something.
    // it should be empty queue
    result = xQueueSend(ch->queue, &HEADER, portMAX_DELAY);
    if (result != pdPASS) {
        // But actually if we put 0, here, will get a assert. why?
        // TODO: decrease it to zero without error
        assert(0);
        goto EXIT;
    }

    while(1) {
        result = xQueueSend(ch->queue, &data, timeout);
        if (result == pdTRUE) {
            break;
        }


        // Now we should take out header we just send, it should not fail in most case,
        // since not body actually get queue
        if (xSemaphoreTake(ch->recv_mutex, 0) != pdTRUE) {
            // if we do fail, which mean some one just come in when we exit
            // so Resume QueueSend, data should be sent real quick
            printf("What a coincidence!!\n");
            timeout = portMAX_DELAY;
            continue;
        }

        int header;
        if (xQueueReceive(ch->queue, &header, 0) != pdTRUE) {
            // No idea if we could get here, unless RTOS has bug,
            // that somebody stole our header,
            // without get mutex
            xSemaphoreGive(ch->recv_mutex);
            assert(0);
        }

        // take header out complete, now safe to exit
        assert(header == HEADER);
        break;
    }
EXIT:
    xSemaphoreGive(ch->send_mutex);
    return result;

}

// Receive data (blocks until sender sends)
BaseType_t channel_recv(Channel* ch, void** data_ptr, TickType_t timeout) {
    if (ch == NULL || data_ptr == NULL) return pdFALSE;

    if (xSemaphoreTake(ch->recv_mutex, timeout) != pdTRUE) {
        return pdFALSE;
    }
    BaseType_t result;
    int header;
    result = xQueueReceive(ch->queue, &header , timeout);
    if (result != pdTRUE) {
        goto EXIT;
    }
    assert(header == HEADER);

    // some one put a header in, we expect real data will be fill very soon
    // and we couldn't handle that if data didn't come, so delay forever
    result = xQueueReceive(ch->queue, data_ptr, portMAX_DELAY);
EXIT:
    xSemaphoreGive(ch->recv_mutex);
    return result;
}

// Destroy channel
void channel_destroy(Channel* ch) {
    if (!ch) {return;}
    vSemaphoreDelete(ch->recv_mutex);
    vSemaphoreDelete(ch->send_mutex);
    vQueueDelete(ch->queue);
    vPortFree(ch);
}

// ==============================================
Channel *ch;
Channel *end;

#define ARRAY_INDEX(ptr, array) ((size_t)((void *)(ptr) - (void *)(array)) / sizeof((array)[0]))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

// Type var[ROW][COL]
#define ARRAY_2D_ROW_INDEX(ptr, array) \
    ((size_t)((void *)(ptr) - (void *)(array)) / (sizeof((array)[0])))

#define ARRAY_2D_COL_INDEX(ptr, array) \
    (((size_t)((void *)(ptr) - (void *)(array)) % (sizeof((array)[0]))) / sizeof((array)[0][0]))

int TASK_ARG[] = {
    300, // TaskSend0
    301, // TaskSend1
    500, // TaskRecv
};


int TASK_PRIO[][ARRAY_SIZE(TASK_ARG)] = {
     // 1. all equal
    {1, 1, 1},

     // 2. one is lowwer
    {0, 1, 1},
    {1, 0, 1},
    {1, 1, 0},

    // 3. one is higger
    {2, 1, 1},
    {1, 2, 1},
    {1, 1, 2},

    // 4. all different
    // [list(x) for x in itertools.permutations((1, 2, 3))]
    {1, 2, 3},
    {1, 3, 2},
    {2, 1, 3},
    {2, 3, 1},
    {3, 1, 2},
    {3, 2, 1},
};

void TaskSend(void *arg) {
    int* para = (int*)arg;
    int id = ARRAY_2D_COL_INDEX(para, TASK_PRIO);
    int delay = *para * 100;
    printf("delay %d, id %d\n", delay, id);

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
    int* para = (int*)arg;
    int id = ARRAY_2D_COL_INDEX(para, TASK_PRIO);
    int delay = *para * 100;
    printf("delay %d, id %d\n", delay, id);

    int data[2] = {0};
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(delay));

        for (int i = 0; i < 2 ; i ++) {
            printf("%d: Recv Start %d\n", id, i);
            channel_recv(ch, &data[i], portMAX_DELAY);

            int recv = data[i];
            printf("%d: Recv End %d -- %d\n", id, i, recv);
        }
        assert_unique(data, ARRAY_SIZE(data));

        channel_send(end, (void *)id, portMAX_DELAY);
        printf("%d: Recv Final\n", id);

        vTaskDelete(NULL);
    }
}

void TaskMain() {
    ch = channel_create();
    end = channel_create();

    int prio_group_index = 0;
    while(1) {
        int *prio_group = TASK_PRIO[prio_group_index];

        printf("Start !! ====================== %d\n", prio_group_index);

        printf("Mini Free Heap: %d bytes\n", xPortGetMinimumEverFreeHeapSize());

        size_t before_heap_size = xPortGetFreeHeapSize();
        printf("Current Free Heap: %d bytes\n", before_heap_size);

        // vTaskDelay(2);
        // TODO: decrease it to zero without error
        // Update1: This isn't need, if we tweak MainTask Prio to lowwest
        // Update2: semm like heap size effect, if we reduce stack size aslo work
        //          add tweak  MainTask Prio to lowwest will also reduce xPortGetMinimumEverFreeHeapSize
        xTaskCreate( TaskSend,
                "TaskSend0",
                configMINIMAL_STACK_SIZE + 256,
                &prio_group[0],
                prio_group[0] + tskIDLE_PRIORITY + 2,
                NULL );

        xTaskCreate( TaskSend,
                "TaskSend1",
                configMINIMAL_STACK_SIZE + 256,
                &prio_group[1],
                prio_group[1] + tskIDLE_PRIORITY + 2,
                NULL );
        xTaskCreate( TaskRecv,
                "TaskRecv",
                configMINIMAL_STACK_SIZE + 256,
                &prio_group[2],
                prio_group[2] + tskIDLE_PRIORITY + 2,
                NULL );

        size_t after_heap_size = xPortGetFreeHeapSize();
        printf("Used Free Heap: %d bytes\n", before_heap_size - after_heap_size);

        int data[ARRAY_SIZE(TASK_ARG)] = {0};
        for (int i = 0; i < ARRAY_SIZE(TASK_ARG) ; i++) {
            channel_recv(end, &data[i], portMAX_DELAY);
            printf("End %d, data -- %d\n",i, data[i]);
        }
        assert_unique(data, ARRAY_SIZE(data));
        printf("End !! == %d\n", prio_group_index);

        prio_group_index = (prio_group_index + 1) % ARRAY_SIZE(TASK_PRIO);
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
            (tskIDLE_PRIORITY + 10 ),
            // It we use a brand new priority other than tskIDLE_PRIORITY
            // xPortGetMinimumEverFreeHeapSize seem like will be reduce
            // Why?
            NULL );

    vTaskStartScheduler();
    while(1);
}
