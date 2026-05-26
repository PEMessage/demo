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
void printHeapAndStack(int line) {
    // 获取堆信息
    size_t totalHeap = configTOTAL_HEAP_SIZE;
    size_t freeHeap = xPortGetFreeHeapSize();
    size_t minFreeHeap = xPortGetMinimumEverFreeHeapSize();

    // 获取当前任务句柄和名称
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
    const char* taskName = (currentTask != NULL) ? pcTaskGetName(currentTask) : "NotInTask";

    if (currentTask != NULL) {
        // 任务上下文：获取剩余栈（高水位线转字节）
        UBaseType_t hwm = uxTaskGetStackHighWaterMark(currentTask);
        size_t stackRemaining = (size_t)hwm * sizeof(StackType_t);
        printf("[%s %d]: TotalHeap=%u, FreeHeap=%u, MinFreeHeap=%u, TaskStackRemaining=%u\n",
               taskName,
               line,
               (unsigned int)totalHeap,
               (unsigned int)freeHeap,
               (unsigned int)minFreeHeap,
               (unsigned int)stackRemaining);
    } else {
        // 非任务上下文（如调度器未启动、中断内等）
        printf("[%s %d]: TotalHeap=%u, FreeHeap=%u, MinFreeHeap=%u\n",
               taskName,
               line,
               (unsigned int)totalHeap,
               (unsigned int)freeHeap,
               (unsigned int)minFreeHeap);
    }
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

    // printf("[rpc_dispatch] Processing: len=%lu, first2byte=[%02X %02X]\n",
    //        req.input->len,
    //        (unsigned char)req.input->data[0],
    //        (unsigned char)req.input->data[1]);

    req.ret = req.func(req.input, req.output);

    // printf("[rpc_dispatch] RPC done, ret=%d, reply back\n", req.ret);
    xQueueSend(xResponseQueue, &done, portMAX_DELAY);
}

// ================================================
// multi RPC extension
// ================================================

#define MULTI_CAP 1024
#define MULTI_REPLY_UNKNOWN UINT32_MAX  // sentinel: server hasn't determined reply length yet

// --- Coroutine primitives (Duff's device style) ---
#define CR_FIELD \
    int line

#define CR_START(ctx)     switch((ctx)->line) { case 0:
#define CR_YIELD(ctx) do { (ctx)->line = __LINE__; return (0); case __LINE__:; } while(0)
#define CR_RESET(ctx)     do { (ctx)->line = 0; } while(0)
#define CR_END(ctx)       } (ctx)->line = 0;

// --- Protocol headers ---
#define MULTI_MAGIC 0xDEC0

/* multi protocol error codes: negative = protocol error, 0 = OK, positive = handler error */
#define MULTI_OK              0
#define MULTI_ERR_BASE     -1100
#define MULTI_ERR_MAGIC     (MULTI_ERR_BASE - 1)  // send hdr magic mismatch
#define MULTI_ERR_SHORT     (MULTI_ERR_BASE - 2)  // input too short for SendHdr
#define MULTI_ERR_OVERSIZE  (MULTI_ERR_BASE - 3)  // total > rx capacity
#define MULTI_ERR_NOMEM     (MULTI_ERR_BASE - 4)  // buffer allocation failed
#define MULTI_ERR_RESP_HDR  (MULTI_ERR_BASE - 5)  // response too short for RecvHdr
#define MULTI_ERR_RESP_MAGIC (MULTI_ERR_BASE - 6) // response magic mismatch
#define MULTI_ERR_RESP_OVERSIZE (MULTI_ERR_BASE - 7) // response payload exceeds output buffer

typedef struct {
    uint16_t magic;    // must be MULTI_MAGIC
    uint32_t offset;   // payload offset in total message
    uint32_t total;    // total message length
    Func     handler;  // target handler (valid on first fragment)
} SendHdr;

typedef struct {
    uint16_t magic;    // must be MULTI_MAGIC
    uint32_t offset;   // payload offset in total reply
    uint32_t total;    // total reply length; MULTI_REPLY_UNKNOWN = not ready yet
    int32_t  status;   // 0 = OK, <0 = error
} RecvHdr;

// --- Server coroutine context ---
typedef struct {
    CR_FIELD;
    Func     handler;   // cached user handler
    uint32_t tx_total;  // reply total length after handler runs
    uint32_t tx_pos;    // bytes already sent in reply
    Message  rx;        // .data allocated on first use, .len = cap
    Message  tx;        // .data allocated on first use, .len = cap
} MultiCr;


static int CrCreateBuffer(MultiCr *cr)
{
    if (!cr->rx.data) {
        cr->rx.data = pvPortMalloc(MULTI_CAP);
        if (!cr->rx.data) return MULTI_ERR_NOMEM;
        cr->rx.len = MULTI_CAP;
    }
    if (!cr->tx.data) {
        cr->tx.data = pvPortMalloc(MULTI_CAP);
        if (!cr->tx.data) return MULTI_ERR_NOMEM;
        cr->tx.len = MULTI_CAP;
    }
    return MULTI_OK;
}

static void CrDeleteBuffer(MultiCr *cr)
{
    if (cr->rx.data) { vPortFree(cr->rx.data); cr->rx.data = NULL; cr->rx.len = 0; }
    if (cr->tx.data) { vPortFree(cr->tx.data); cr->tx.data = NULL; cr->tx.len = 0; }
}

// --- Helper: assemble RecvHdr + optional payload into output Message ---
static void reply(Message *output, RecvHdr *rh, const char *payload, uint32_t plen)
{
    memcpy(output->data, rh, sizeof(*rh));
    if (plen) memcpy(output->data + sizeof(*rh), payload, plen);
    output->len = sizeof(*rh) + plen;
}

static int parseSendHdr(Message *input, SendHdr *sh) {
    if (input->len < sizeof(*sh)) {
        return MULTI_ERR_SHORT;
    }
    memcpy(sh, input->data, sizeof(*sh));
    if (sh->magic != MULTI_MAGIC) {
        return MULTI_ERR_MAGIC;
    }
    return 0;
}

// --- Server coroutine body ---
static int multi_handler_cr(MultiCr *cr, Message *input, Message *output)
{
    SendHdr sh;
    RecvHdr rh;

    int ret = parseSendHdr(input, &sh);
    if(ret) {
        CrDeleteBuffer(cr);
        cr->line = 0;
        return ret;
    }
    // Allow Sender force reset state
    if (sh.offset == 0 && sh.total != 0) {
        cr->line = 0;
    }

    CR_START(cr);

    /* -------- init -------- */
    cr->tx_total = 0;
    cr->tx_pos = 0;
    cr->handler = sh.handler;
    if (CrCreateBuffer(cr) != MULTI_OK) {
        CrDeleteBuffer(cr);
        CR_RESET(cr);
        return MULTI_ERR_NOMEM;
    }

    /* -------- receive all fragments -------- */
    for (;;) {
        if (sh.total > cr->rx.len) {
            CrDeleteBuffer(cr);
            CR_RESET(cr);
            return MULTI_ERR_OVERSIZE;
        }

        uint32_t plen = (input->len > sizeof(SendHdr)) ? (input->len - sizeof(SendHdr)) : 0;
        if (plen) memcpy(cr->rx.data + sh.offset, input->data + sizeof(SendHdr), plen);

        if (sh.offset + plen >= sh.total) break;

        rh = (RecvHdr){ .magic = MULTI_MAGIC, .offset = 0, .total = MULTI_REPLY_UNKNOWN, .status = 0 };
        reply(output, &rh, NULL, 0);
        CR_YIELD(cr);
    }

    /* -------- run user handler -------- */
    {
        Message req  = { .data = cr->rx.data, .len = sh.total };
        Message resp = { .data = cr->tx.data, .len = cr->tx.len };
        cr->handler(&req, &resp);
        cr->tx_total = resp.len;
        cr->tx_pos   = 0;
    }

    /* -------- transmit all reply fragments -------- */
    for (;;) {
        uint32_t remain = cr->tx_total - cr->tx_pos;
        uint32_t room   = MESSAGE_MAX_LEN - sizeof(RecvHdr);
        uint32_t chunk  = (remain > room) ? room : remain;

        rh = (RecvHdr){ .magic = MULTI_MAGIC, .offset = cr->tx_pos, .total = cr->tx_total, .status = 0 };
        reply(output, &rh, cr->tx.data + cr->tx_pos, chunk);
        cr->tx_pos += chunk;

        if (cr->tx_pos >= cr->tx_total) {
            CrDeleteBuffer(cr);
            CR_RESET(cr);
            return 0;
        }
        CR_YIELD(cr);
    }

    // should not get here if all success
    CrDeleteBuffer(cr);
    CR_END(cr);

    return 0;
}

// --- Public server entry ---
int multi_handler(Message *input, Message *output)
{
    static MultiCr mcr = {0};
    return multi_handler_cr(&mcr, input, output);
}

// --- Client: symmetric multi RPC ---
int rpc_call_multi(Func func, Message *input, Message *output)
{
    char s[MESSAGE_MAX_LEN];
    char r[MESSAGE_MAX_LEN];

    uint32_t tx = 0, rx = 0, rx_need = MULTI_REPLY_UNKNOWN;

    while (tx < input->len || rx < rx_need) {
        uint32_t room = MESSAGE_MAX_LEN - sizeof(SendHdr);
        uint32_t n = input->len - tx;
        if (n > room) n = room;

        SendHdr sh = { .magic = MULTI_MAGIC, .offset = tx, .total = input->len, .handler = func };
        memcpy(s, &sh, sizeof(sh));
        if (n) memcpy(s + sizeof(sh), input->data + tx, n);

        Message sm = { sizeof(sh) + n, s };
        Message rm = { MESSAGE_MAX_LEN, r };
        int ret = rpc_call(multi_handler, &sm, &rm);
        if (ret) return ret;


        if (rm.len < sizeof(RecvHdr)) return MULTI_ERR_RESP_HDR;
        RecvHdr rh;
        memcpy(&rh, r, sizeof(rh));
        if (rh.magic != MULTI_MAGIC) return MULTI_ERR_RESP_MAGIC;
        if (rh.status != 0) return rh.status;
        if (rh.total != MULTI_REPLY_UNKNOWN) rx_need = rh.total;

        uint32_t m = rm.len - sizeof(rh);
        if (rx + m > output->len) return MULTI_ERR_RESP_OVERSIZE;
        if (m) memcpy(output->data + rx, r + sizeof(rh), m);
        rx += m;
        tx += n;
    }

    output->len = rx;
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

static int test_echo(uint32_t input_len, uint32_t output_len, uint32_t seed) {
    Message *input  = MessageCreate(input_len);
    Message *output = MessageCreate(output_len);
    if (!input || !output) {
        printf("[test_echo] malloc failed for len=%lu\n", (unsigned long)input_len);
        MessageDelete(input);
        MessageDelete(output);
        return -1;
    }

    fill_input(input->data, input->len, seed);
    memset(output->data, 0xAC, output->len);

    printf("[test_echo] --> RPC multi request (%lu bytes), seed=%lu\n",
           (unsigned long)input->len, (unsigned long)seed);

    int ret = rpc_call_multi(echo_handler, input, output);

    int ok = (ret == 0 && output->len == input->len &&
              memcmp(input->data, output->data, input->len) == 0);
    printf("[test_echo] <-- RPC multi response: ret=%d, recv_len=%lu, match=%s\n",
           ret, (unsigned long)output->len, ok ? "PASS" : "FAIL");

    MessageDelete(input);
    MessageDelete(output);

    return ok ? 0 : (ret ? ret : -1);
}

void TaskSend(void *pvParameters) {
    (void)pvParameters;
    uint32_t counter = 0;

    printHeapAndStack(__LINE__);
    for (;;) {
        // printHeapAndStack(__LINE__);

        int ret;

        /* 0-byte payload */
        ret = test_echo(0, MULTI_CAP, counter);
        printf("[TaskSend] test 0: ret=%d (expect 0)\n\n", ret);

        /* MULTI_CAP - 1 */
        ret = test_echo(MULTI_CAP - 1, MULTI_CAP, counter);
        printf("[TaskSend] test MULTI_CAP-1: ret=%d (expect 0)\n\n", ret);

        /* MULTI_CAP */
        ret = test_echo(MULTI_CAP, MULTI_CAP, counter);
        printf("[TaskSend] test MULTI_CAP: ret=%d (expect 0)\n\n", ret);

        /* MULTI_CAP + 1 -> server rejects as oversize */
        ret = test_echo(MULTI_CAP + 1, MULTI_CAP, counter);
        printf("[TaskSend] test MULTI_CAP+1: ret=%d (expect %d)\n\n",
               ret, MULTI_ERR_OVERSIZE);

        /* output buffer too small -> client rejects as resp oversize */
        ret = test_echo(MULTI_CAP, 512, counter);
        printf("[TaskSend] test out=512: ret=%d (expect %d)\n\n",
               ret, MULTI_ERR_RESP_OVERSIZE);


        counter++;
        vTaskDelay(pdMS_TO_TICKS(1000));
        while(1);

        // printHeapAndStack(__LINE__);
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

    printHeapAndStack(__LINE__);
    xTaskCreate(TaskSend,
                "TaskSend",
                configMINIMAL_STACK_SIZE * 2,
                NULL,
                (tskIDLE_PRIORITY + 1),
                NULL);
    xTaskCreate(TaskRecv,
                "TaskRecv",
                configMINIMAL_STACK_SIZE * 2,
                NULL,
                (tskIDLE_PRIORITY + 1),
                NULL);

    vTaskStartScheduler();
    while (1);
}
