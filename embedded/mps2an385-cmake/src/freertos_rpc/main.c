#include <stdio.h>
#include <string.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// ================================================
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

// ================================================
#define QUEUE_LENGTH 5
#define MESSAGE_MAX_LEN 64

typedef struct {
    uint32_t len;
    char *data;
} Message;

typedef int (*Func)(Message *input, Message *output);

static QueueHandle_t xRequestQueue = NULL;
static QueueHandle_t xResponseQueue = NULL;

// ================================================
int echo_handler(Message *input, Message *output) {
    if (!input || !output || !input->data || !output->data) {
        return -1;
    }

    uint32_t avail = output->len;
    uint32_t copy_len = input->len;
    if (copy_len >= avail) {
        copy_len = avail - 1;
    }

    memcpy(output->data, input->data, copy_len);
    output->data[copy_len] = '\0';
    output->len = copy_len;

    return 0;
}

// ================================================
typedef struct {
    Message *input;
    Message *output;
    Func     func;
    int      ret;
} RpcRequest;

int rpc_call(Func func, Message *input, Message *output) {
    RpcRequest req = {
        .input  = input,
        .output = output,
        .func   = func,
        .ret    = 0,
    };
    uint32_t done;

    xQueueSend(xRequestQueue, &req, portMAX_DELAY);
    xQueueReceive(xResponseQueue, &done, portMAX_DELAY);
    return req.ret;
}
// ================================================


void TaskSend(void *pvParameters) {
    (void)pvParameters;
    Message input;
    Message output;
    uint32_t counter = 0;

    for (;;) {
        input.data = pvPortMalloc(MESSAGE_MAX_LEN);
        output.data = pvPortMalloc(MESSAGE_MAX_LEN);
        if (input.data == NULL || output.data == NULL) {
            printf("[TaskSend] malloc failed\n");
            if (input.data)  vPortFree(input.data);
            if (output.data) vPortFree(output.data);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        input.len = snprintf(input.data, MESSAGE_MAX_LEN,
                             "Req #%lu from Send", counter++);
        output.len = MESSAGE_MAX_LEN;  // 告知缓冲区大小

        printf("[TaskSend] --> RPC request: len=%lu, data=%s\n",
               input.len, input.data);

        int ret = rpc_call(echo_handler, &input, &output);

        printf("[TaskSend] <-- RPC response: ret=%d, len=%lu, data=%.*s\n",
               ret, output.len, (int)output.len, output.data);

        vPortFree(input.data);
        vPortFree(output.data);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void TaskRecv(void *pvParameters) {
    (void)pvParameters;
    RpcRequest req;
    uint32_t done = 1;

    for (;;) {
        if (xQueueReceive(xRequestQueue, &req, portMAX_DELAY) == pdPASS) {
            printf("[TaskRecv] Processing: len=%lu, data=%.*s\n",
                   req.input->len, (int)req.input->len, req.input->data);

            req.ret = req.func(req.input, req.output);

            printf("[TaskRecv] RPC done, ret=%d, reply back\n", req.ret);
            xQueueSend(xResponseQueue, &done, portMAX_DELAY);
        }
    }
}

int main() {
    setup();
    printf("Init complete\n");

    xRequestQueue  = xQueueCreate(QUEUE_LENGTH, sizeof(RpcRequest));
    xResponseQueue = xQueueCreate(QUEUE_LENGTH, sizeof(uint32_t));
    if (xRequestQueue == NULL || xResponseQueue == NULL) {
        printf("Queue creation failed!\n");
        while (1);
    }

    xTaskCreate(TaskSend,
                "TaskSend",
                configMINIMAL_STACK_SIZE + 512,
                NULL,
                (tskIDLE_PRIORITY + 1),
                NULL);
    xTaskCreate(TaskRecv,
                "TaskRecv",
                configMINIMAL_STACK_SIZE + 512,
                NULL,
                (tskIDLE_PRIORITY + 1),
                NULL);

    vTaskStartScheduler();
    while (1);
}
