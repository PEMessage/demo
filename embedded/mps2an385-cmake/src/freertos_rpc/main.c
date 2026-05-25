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
// multi RPC extension
// ================================================

#define FRAG_FLAG_FIRST (1 << 0)
#define FRAG_FLAG_LAST  (1 << 1)
#define FRAG_FLAG_POLL  (1 << 2)
#define MULTI_MESSAGE_MAX_LEN 256

typedef struct {
    uint8_t flags;
    uint8_t seq;
    Func    handler;
} FragHeader;

typedef enum {
    MULTI_IDLE,
    MULTI_RX,
    MULTI_TX
} MultiState;

typedef struct {
    MultiState state;
    char     rx_buf[MULTI_MESSAGE_MAX_LEN];
    uint32_t rx_len;
    char     tx_buf[MULTI_MESSAGE_MAX_LEN];
    uint32_t tx_len;
    uint32_t tx_offset;
} MultiCtx;

static int multi_send_next_fragment(MultiCtx *ctx, Message *output) {
    if (ctx->state != MULTI_TX) {
        output->data[0] = FRAG_FLAG_LAST;
        output->data[1] = 0;
        memcpy(output->data + sizeof(FragHeader), "ERR", 4);
        output->len = sizeof(FragHeader) + 4;
        return -1;
    }

    uint32_t remain = ctx->tx_len - ctx->tx_offset;
    uint32_t payload_len = remain > (MESSAGE_MAX_LEN - sizeof(FragHeader))
                           ? (MESSAGE_MAX_LEN - sizeof(FragHeader))
                           : remain;

    FragHeader hdr = {
        .flags = (remain <= (MESSAGE_MAX_LEN - sizeof(FragHeader))) ? FRAG_FLAG_LAST : 0,
        .seq   = 0,
        .handler = NULL,
    };
    memcpy(output->data, &hdr, sizeof(hdr));
    memcpy(output->data + sizeof(hdr), ctx->tx_buf + ctx->tx_offset, payload_len);
    output->len = sizeof(hdr) + payload_len;

    ctx->tx_offset += payload_len;
    if (ctx->tx_offset >= ctx->tx_len) {
        ctx->state = MULTI_IDLE;
    }
    return 0;
}

static int multi_handler_internal(MultiCtx *ctx, Message *input, Message *output) {
    FragHeader hdr;
    if (input->len < sizeof(hdr)) {
        return -1;
    }
    memcpy(&hdr, input->data, sizeof(hdr));

    if (hdr.flags & FRAG_FLAG_POLL) {
        return multi_send_next_fragment(ctx, output);
    }

    if (hdr.flags & FRAG_FLAG_FIRST) {
        ctx->state = MULTI_RX;
        ctx->rx_len = 0;
    }

    if (ctx->state != MULTI_RX) {
        FragHeader err_hdr = { .flags = FRAG_FLAG_LAST, .seq = 0, .handler = NULL };
        memcpy(output->data, &err_hdr, sizeof(err_hdr));
        memcpy(output->data + sizeof(err_hdr), "NACK", 5);
        output->len = sizeof(err_hdr) + 5;
        return -1;
    }

    uint32_t payload_len = input->len - sizeof(hdr);
    if (ctx->rx_len + payload_len > sizeof(ctx->rx_buf)) {
        ctx->state = MULTI_IDLE;
        FragHeader err_hdr = { .flags = FRAG_FLAG_LAST, .seq = 0, .handler = NULL };
        memcpy(output->data, &err_hdr, sizeof(err_hdr));
        memcpy(output->data + sizeof(err_hdr), "OVFL", 5);
        output->len = sizeof(err_hdr) + 5;
        return -1;
    }

    memcpy(ctx->rx_buf + ctx->rx_len, input->data + sizeof(hdr), payload_len);
    ctx->rx_len += payload_len;

    if (hdr.flags & FRAG_FLAG_LAST) {
        Message echo_in  = { .len = ctx->rx_len, .data = ctx->rx_buf };
        Message echo_out = { .len = sizeof(ctx->tx_buf), .data = ctx->tx_buf };
        hdr.handler(&echo_in, &echo_out);

        ctx->tx_len    = echo_out.len;
        ctx->tx_offset = 0;
        ctx->state     = MULTI_TX;

        return multi_send_next_fragment(ctx, output);
    }

    FragHeader ack_hdr = { .flags = FRAG_FLAG_LAST, .seq = 0, .handler = NULL };
    memcpy(output->data, &ack_hdr, sizeof(ack_hdr));
    memcpy(output->data + sizeof(ack_hdr), "ACK", 4);
    output->len = sizeof(ack_hdr) + 4;
    return 0;
}

int multi_handler(Message *input, Message *output) {
    static MultiCtx ctx;
    return multi_handler_internal(&ctx, input, output);
}

int rpc_call_multi(Func func, Message *input, Message *output) {
    char in_data[MESSAGE_MAX_LEN];
    char out_data[MESSAGE_MAX_LEN];
    Message in = { .data = in_data };
    Message out = { .data = out_data };

    uint32_t send_len = input->len;
    uint32_t send_offset = 0;
    uint8_t msg_id = 0;
    uint32_t recv_max = output->len;

    while (send_offset < send_len) {
        uint32_t payload_len = (send_len - send_offset) > (MESSAGE_MAX_LEN - sizeof(FragHeader))
                               ? (MESSAGE_MAX_LEN - sizeof(FragHeader))
                               : (send_len - send_offset);

        FragHeader hdr = {
            .flags   = 0,
            .seq     = msg_id,
            .handler = func,
        };
        if (send_offset == 0) hdr.flags |= FRAG_FLAG_FIRST;
        if (send_offset + payload_len >= send_len) hdr.flags |= FRAG_FLAG_LAST;

        memcpy(in_data, &hdr, sizeof(hdr));
        memcpy(in_data + sizeof(hdr), input->data + send_offset, payload_len);
        in.len = sizeof(hdr) + payload_len;
        out.len = MESSAGE_MAX_LEN;

        int ret = rpc_call(multi_handler, &in, &out);
        if (ret != 0) return ret;

        if (send_offset + payload_len >= send_len) {
            uint32_t recv_offset = 0;
            FragHeader resp_hdr;
            memcpy(&resp_hdr, out_data, sizeof(resp_hdr));

            while (1) {
                uint32_t resp_payload = out.len > sizeof(FragHeader) ? out.len - sizeof(FragHeader) : 0;
                if (recv_offset + resp_payload > recv_max) {
                    resp_payload = recv_max - recv_offset;
                }
                memcpy(output->data + recv_offset, out_data + sizeof(FragHeader), resp_payload);
                recv_offset += resp_payload;

                if (resp_hdr.flags & FRAG_FLAG_LAST) break;

                FragHeader poll_hdr = {
                    .flags   = FRAG_FLAG_POLL,
                    .seq     = msg_id,
                    .handler = func,
                };
                memcpy(in_data, &poll_hdr, sizeof(poll_hdr));
                in.len = sizeof(poll_hdr);
                out.len = MESSAGE_MAX_LEN;

                ret = rpc_call(multi_handler, &in, &out);
                if (ret != 0) return ret;
                memcpy(&resp_hdr, out_data, sizeof(resp_hdr));
            }

            output->len = recv_offset;
            return 0;
        }

        send_offset += payload_len;
    }

    return 0;
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
        char input_buf[200];
        char output_buf[MULTI_MESSAGE_MAX_LEN];
        uint32_t send_len = sizeof(input_buf);

        fill_input(input_buf, send_len, counter);
        memset(output_buf, 0xAC, sizeof(output_buf));

        Message input  = { .len = send_len, .data = input_buf };
        Message output = { .len = sizeof(output_buf), .data = output_buf };

        printf("[TaskSend] --> RPC multi request (%lu bytes), seed=%lu\n",
               input.len, counter);

        int ret = rpc_call_multi(echo_handler, &input, &output);

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
