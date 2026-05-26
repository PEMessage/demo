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
Message *MessageCreate(uint32_t size) {
    Message *msg = pvPortMalloc(sizeof(Message));
    if (!msg) return NULL;
    msg->data = pvPortMalloc(size);
    if (!msg->data) {
        vPortFree(msg);
        return NULL;
    }
    msg->len = size;
    return msg;
}

void MessageDelete(Message *message) {
    if (!message) return;
    if (message->data) {
        vPortFree(message->data);
    }
    vPortFree(message);
}

uint32_t MessageCopy(Message *dest, uint32_t destpos,
                     const Message *src, uint32_t srcpos,
                     uint32_t size) {
    if (!dest || !src || !dest->data || !src->data) return 0;
    if (destpos >= dest->len || srcpos >= src->len) return 0;

    uint32_t avail_src  = src->len - srcpos;
    uint32_t avail_dest = dest->len - destpos;
    uint32_t copy_len   = size;
    if (copy_len > avail_src)  copy_len = avail_src;
    if (copy_len > avail_dest) copy_len = avail_dest;

    memcpy(dest->data + destpos, src->data + srcpos, copy_len);
    return copy_len;
}

// ================================================
int echo_handler(Message *input, Message *output) {
    if (!input || !output || !input->data || !output->data) {
        return -1;
    }
    output->len = MessageCopy(output, 0, input, 0, input->len);
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
    Message  rx;      // len = capacity, data = rx_buf
    Message  tx;      // len = capacity 或 handler 设置的实际长度
    uint32_t rx_pos;
    uint32_t tx_pos;
} MultiCtx;

static int multi_send_next_fragment(MultiCtx *ctx, Message *output) {
    if (ctx->state != MULTI_TX) {
        FragHeader hdr = { .flags = FRAG_FLAG_LAST, .seq = 0, .handler = NULL };
        Message hdr_msg = { .len = sizeof(hdr), .data = (char*)&hdr };
        Message err = { .len = 4, .data = "ERR" };
        MessageCopy(output, 0, &hdr_msg, 0, sizeof(hdr));
        MessageCopy(output, sizeof(hdr), &err, 0, 4);
        output->len = sizeof(hdr) + 4;
        return -1;
    }

    uint32_t remain = ctx->tx.len - ctx->tx_pos;
    uint32_t payload_len = remain > (MESSAGE_MAX_LEN - sizeof(FragHeader))
                           ? (MESSAGE_MAX_LEN - sizeof(FragHeader))
                           : remain;

    FragHeader hdr = {
        .flags = (remain <= (MESSAGE_MAX_LEN - sizeof(FragHeader))) ? FRAG_FLAG_LAST : 0,
        .seq   = 0,
        .handler = NULL,
    };
    Message hdr_msg = { .len = sizeof(hdr), .data = (char*)&hdr };
    MessageCopy(output, 0, &hdr_msg, 0, sizeof(hdr));
    MessageCopy(output, sizeof(hdr), &ctx->tx, ctx->tx_pos, payload_len);
    output->len = sizeof(hdr) + payload_len;

    ctx->tx_pos += payload_len;
    if (ctx->tx_pos >= ctx->tx.len) {
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
        ctx->rx_pos = 0;
    }

    if (ctx->state != MULTI_RX) {
        FragHeader err_hdr = { .flags = FRAG_FLAG_LAST, .seq = 0, .handler = NULL };
        Message hdr_msg = { .len = sizeof(err_hdr), .data = (char*)&err_hdr };
        Message nack = { .len = 5, .data = "NACK" };
        MessageCopy(output, 0, &hdr_msg, 0, sizeof(err_hdr));
        MessageCopy(output, sizeof(err_hdr), &nack, 0, 5);
        output->len = sizeof(err_hdr) + 5;
        return -1;
    }

    Message src = { .len = input->len, .data = input->data };
    uint32_t copied = MessageCopy(&ctx->rx, ctx->rx_pos, &src, sizeof(hdr), input->len - sizeof(hdr));
    ctx->rx_pos += copied;
    if (ctx->rx_pos > MULTI_MESSAGE_MAX_LEN) {
        ctx->state = MULTI_IDLE;
        FragHeader err_hdr = { .flags = FRAG_FLAG_LAST, .seq = 0, .handler = NULL };
        Message hdr_msg = { .len = sizeof(err_hdr), .data = (char*)&err_hdr };
        Message ovfl = { .len = 5, .data = "OVFL" };
        MessageCopy(output, 0, &hdr_msg, 0, sizeof(err_hdr));
        MessageCopy(output, sizeof(err_hdr), &ovfl, 0, 5);
        output->len = sizeof(err_hdr) + 5;
        return -1;
    }

    if (hdr.flags & FRAG_FLAG_LAST) {
        Message echo_in = { .len = ctx->rx_pos, .data = ctx->rx.data };
        ctx->tx.len = MULTI_MESSAGE_MAX_LEN; // 重置 capacity
        hdr.handler(&echo_in, &ctx->tx);

        ctx->tx_pos = 0;
        ctx->state  = MULTI_TX;

        return multi_send_next_fragment(ctx, output);
    }

    FragHeader ack_hdr = { .flags = FRAG_FLAG_LAST, .seq = 0, .handler = NULL };
    Message hdr_msg = { .len = sizeof(ack_hdr), .data = (char*)&ack_hdr };
    Message ack = { .len = 4, .data = "ACK" };
    MessageCopy(output, 0, &hdr_msg, 0, sizeof(ack_hdr));
    MessageCopy(output, sizeof(ack_hdr), &ack, 0, 4);
    output->len = sizeof(ack_hdr) + 4;
    return 0;
}

int multi_handler(Message *input, Message *output) {
    static char rx_buf[MULTI_MESSAGE_MAX_LEN];
    static char tx_buf[MULTI_MESSAGE_MAX_LEN];
    static MultiCtx ctx;
    if (ctx.rx.data == NULL) {
        ctx.rx.data = rx_buf;
        ctx.rx.len  = MULTI_MESSAGE_MAX_LEN;
        ctx.tx.data = tx_buf;
        ctx.tx.len  = MULTI_MESSAGE_MAX_LEN;
    }
    return multi_handler_internal(&ctx, input, output);
}

int rpc_call_multi(Func func, Message *input, Message *output) {
    char in_data[MESSAGE_MAX_LEN];
    char out_data[MESSAGE_MAX_LEN];
    Message in  = { .len = MESSAGE_MAX_LEN, .data = in_data };
    Message out = { .len = MESSAGE_MAX_LEN, .data = out_data };

    uint32_t send_len = input->len;
    uint32_t send_offset = 0;
    uint8_t msg_id = 0;

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

        Message hdr_msg = { .len = sizeof(hdr), .data = (char*)&hdr };
        in.len = MESSAGE_MAX_LEN;
        MessageCopy(&in, 0, &hdr_msg, 0, sizeof(hdr));
        MessageCopy(&in, sizeof(hdr), input, send_offset, payload_len);
        in.len = sizeof(hdr) + payload_len;
        out.len = MESSAGE_MAX_LEN;

        int ret = rpc_call(multi_handler, &in, &out);
        if (ret != 0) return ret;

        if (send_offset + payload_len >= send_len) {
            uint32_t recv_offset = 0;
            FragHeader resp_hdr;
            memcpy(&resp_hdr, out_data, sizeof(resp_hdr));

            while (1) {
                uint32_t copied = MessageCopy(output, recv_offset, &out, sizeof(FragHeader), out.len);
                recv_offset += copied;

                if (resp_hdr.flags & FRAG_FLAG_LAST) break;

                FragHeader poll_hdr = {
                    .flags   = FRAG_FLAG_POLL,
                    .seq     = msg_id,
                    .handler = func,
                };
                Message poll_msg = { .len = sizeof(poll_hdr), .data = (char*)&poll_hdr };
                in.len = MESSAGE_MAX_LEN;
                MessageCopy(&in, 0, &poll_msg, 0, sizeof(poll_hdr));
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
        Message *input  = MessageCreate(200);
        Message *output = MessageCreate(MULTI_MESSAGE_MAX_LEN);
        if (!input || !output) {
            printf("[TaskSend] malloc failed\n");
            MessageDelete(input);
            MessageDelete(output);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        fill_input(input->data, input->len, counter);
        memset(output->data, 0xAC, output->len);

        printf("[TaskSend] --> RPC multi request (%lu bytes), seed=%lu\n",
               input->len, counter);

        int ret = rpc_call_multi(echo_handler, input, output);

        int ok = (ret == 0 && output->len == input->len &&
                  memcmp(input->data, output->data, input->len) == 0);
        printf("[TaskSend] <-- RPC multi response: ret=%d, recv_len=%lu, match=%s\n",
               ret, output->len, ok ? "PASS" : "FAIL");

        MessageDelete(input);
        MessageDelete(output);

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
