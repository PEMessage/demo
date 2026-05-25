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
    if (copy_len > avail) {
        copy_len = avail;
    }

    memcpy(output->data, input->data, copy_len);
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

void rpc_dispatch(void) {
    RpcRequest req;
    uint32_t done = 1;

    xQueueReceive(xRequestQueue, &req, portMAX_DELAY);

    printf("[rpc_dispatch] Processing: len=%lu, first2byte=[%02X %02X]\n",
           req.input->len,
           (unsigned char)req.input->data[0],
           (unsigned char)req.input->data[1]);

    req.ret = req.func(req.input, req.output);

    printf("[rpc_dispatch] RPC done, ret=%d, reply back\n", req.ret);
    xQueueSend(xResponseQueue, &done, portMAX_DELAY);
}


// ================================================
// tasks
// ================================================

static void fill_input(char *data, uint32_t len, uint32_t seed) {
    for (uint32_t i = 0; i < len; i++) {
        data[i] = (char)(seed + i);
    }
}

void TaskSend(void *pvParameters) {
    (void)pvParameters;
    uint32_t counter = 0;

    for (;;) {
        char input_buf[MESSAGE_MAX_LEN];
        char output_buf[MESSAGE_MAX_LEN];
        uint32_t send_len = sizeof(input_buf);

        fill_input(input_buf, send_len, counter);
        memset(output_buf, 0xAC, sizeof(output_buf));

        Message input  = { .len = send_len, .data = input_buf };
        Message output = { .len = sizeof(output_buf), .data = output_buf };

        printf("[TaskSend] --> RPC multi request (%lu bytes), seed=%lu\n",
               input.len, counter);

        int ret = rpc_call(echo_handler, &input, &output);

        int ok = (ret == 0 && output.len == input.len &&
                  memcmp(input.data, output.data, input.len) == 0);
        printf("[TaskSend] <-- RPC multi response: ret=%d, recv_len=%lu, match=%s\n",
               ret, output.len, ok ? "PASS" : "FAIL");

        counter++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void TaskRecv(void *pvParameters) {
    (void)pvParameters;
    for (;;) {
        rpc_dispatch();
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
