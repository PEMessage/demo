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

#define MULTI_CAP 256

// --- Coroutine primitives (Duff's device style) ---
#define CR_FIELD \
    int line

#define CR_START(ctx)     switch((ctx)->line) { case 0:
#define CR_YIELD(ctx, rv) do { (ctx)->line = __LINE__; return (rv); case __LINE__:; } while(0)
#define CR_RESET(ctx)     do { (ctx)->line = 0; } while(0)
#define CR_END(ctx)       } (ctx)->line = 0;

// --- Protocol headers ---
typedef struct {
    uint32_t offset;   // payload offset in total message
    uint32_t total;    // total message length
    Func     handler;  // target handler (valid on first fragment)
} SendHdr;

typedef struct {
    uint32_t offset;   // payload offset in total reply
    uint32_t total;    // total reply length; (uint32_t)-1 = not ready
    int32_t  status;   // 0 = OK, <0 = error
} RecvHdr;

// --- Server coroutine context ---
typedef struct {
    CR_FIELD;
    Func     handler;   // cached user handler
    uint32_t tx_total;  // reply total length after handler runs
    uint32_t tx_pos;    // bytes already sent in reply
} MultiCr;

// --- Server static buffers ---
static char    rx_buf[MULTI_CAP];
static char    tx_buf[MULTI_CAP];
static MultiCr mcr   = {0};

// --- Helper: assemble RecvHdr + optional payload into output Message ---
static void reply(Message *output, RecvHdr *rh, const Message *payload)
{
    Message hdr = { .data = (char*)rh, .len = sizeof(*rh) };
    uint32_t copied = MessageCopy(output, 0, &hdr, 0, sizeof(*rh));
    if (payload && payload->len) {
        copied += MessageCopy(output, sizeof(*rh), payload, 0, payload->len);
    }
    output->len = copied;
}

// --- Server coroutine body ---
static int multi_handler_cr(MultiCr *cr, Message *input, Message *output)
{
    SendHdr sh;
    RecvHdr rh;

    CR_START(cr);

    /* -------- receive all fragments -------- */
    for (;;) {
        if (input->len < sizeof(SendHdr)) return -1;
        memcpy(&sh, input->data, sizeof(sh));

        if (sh.total > MULTI_CAP) return -1;

        if (sh.offset == 0 && sh.total > 0) {
            cr->handler = sh.handler;
        }

        uint32_t plen = (input->len > sizeof(SendHdr)) ? (input->len - sizeof(SendHdr)) : 0;
        if (plen) {
            Message src = { .data = input->data, .len = input->len };
            Message dst = { .data = rx_buf, .len = MULTI_CAP };
            MessageCopy(&dst, sh.offset, &src, sizeof(SendHdr), plen);
        }

        if (sh.offset + plen >= sh.total) break;

        rh = (RecvHdr){ .offset = 0, .total = (uint32_t)-1, .status = 0 };
        reply(output, &rh, NULL);
        CR_YIELD(cr, 0);
    }

    /* -------- run user handler -------- */
    {
        Message req  = { .data = rx_buf, .len = sh.total };
        Message resp = { .data = tx_buf, .len = MULTI_CAP };
        cr->handler(&req, &resp);
        cr->tx_total = resp.len;
        cr->tx_pos   = 0;
    }

    /* -------- transmit all reply fragments -------- */
    for (;;) {
        uint32_t remain = cr->tx_total - cr->tx_pos;
        uint32_t room   = MESSAGE_MAX_LEN - sizeof(RecvHdr);
        uint32_t chunk  = (remain > room) ? room : remain;

        rh = (RecvHdr){ .offset = cr->tx_pos, .total = cr->tx_total, .status = 0 };
        Message payload = { .data = tx_buf + cr->tx_pos, .len = chunk };
        reply(output, &rh, &payload);
        cr->tx_pos += chunk;

        if (cr->tx_pos >= cr->tx_total) {
            CR_RESET(cr);
            return 0;
        }
        CR_YIELD(cr, 0);
    }

    CR_END(cr);
    return -1;
}

// --- Public server entry ---
int multi_handler(Message *input, Message *output)
{
    return multi_handler_cr(&mcr, input, output);
}

// --- Client: symmetric multi RPC ---
int rpc_call_multi(Func func, Message *input, Message *output)
{
    char in_buf[MESSAGE_MAX_LEN];
    char out_buf[MESSAGE_MAX_LEN];

    uint32_t send_off = 0;
    uint32_t send_tot = input->len;
    uint32_t recv_off = 0;
    uint32_t recv_tot = (uint32_t)-1;
    int      result   = 0;

    Message in  = { .data = in_buf,  .len = MESSAGE_MAX_LEN };
    Message out = { .data = out_buf, .len = MESSAGE_MAX_LEN };

    while (send_off < send_tot || recv_off < recv_tot) {
        /* assemble SendHdr + payload */
        SendHdr sh = { .offset = send_off, .total = send_tot, .handler = func };
        uint32_t max_pl = MESSAGE_MAX_LEN - sizeof(SendHdr);
        uint32_t plen   = 0;

        if (send_off < send_tot) {
            plen = send_tot - send_off;
            if (plen > max_pl) plen = max_pl;
        }

        Message hdr_msg = { .data = (char*)&sh, .len = sizeof(sh) };
        in.len = MESSAGE_MAX_LEN;
        MessageCopy(&in, 0, &hdr_msg, 0, sizeof(sh));
        if (plen) {
            MessageCopy(&in, sizeof(sh), input, send_off, plen);
        }
        in.len = sizeof(sh) + plen;

        /* swap */
        out.len = MESSAGE_MAX_LEN;
        result = rpc_call(multi_handler, &in, &out);
        if (result != 0) return result;

        /* parse RecvHdr */
        RecvHdr rh;
        if (out.len < sizeof(rh)) return -1;
        memcpy(&rh, out.data, sizeof(rh));
        if (rh.status != 0) return rh.status;
        if (rh.total != (uint32_t)-1) recv_tot = rh.total;

        /* copy any payload to output (invariant 3) */
        uint32_t rplen = (out.len > sizeof(rh)) ? (out.len - sizeof(rh)) : 0;
        if (rplen) {
            recv_off += MessageCopy(output, recv_off, &out, sizeof(rh), rplen);
        }

        send_off += plen;
    }

    output->len = recv_off;
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
        Message *output = MessageCreate(MULTI_CAP);
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
