#include <stdio.h>
#include <string.h>
#include <assert.h>
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
#define ARRAY_INDEX(ptr, array) ((size_t)((void *)(ptr) - (void *)(array)) / sizeof((array)[0]))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define __same_type(a, b) __builtin_types_compatible_p(__typeof__(a), __typeof__(b))

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    // C11 or later: use static_assert
    #define ASSERT_STATIC(expr, msg) static_assert(expr, msg)
#elif defined(__GNUC__) || defined(__clang__)
    // GCC or Clang: use _Static_assert
    #define ASSERT_STATIC(expr, msg) _Static_assert(expr, msg)
#else
    // Fallback for older compilers: use a hack with typedef and array size
    #define ASSERT_STATIC(expr, msg) typedef char static_assert_##msg[(expr) ? 1 : -1]
#endif

#define CONTAINER_OF(ptr, type, member) ({				\
	void *__mptr = (void *)(ptr);					\
	ASSERT_STATIC(__same_type(*(ptr), ((type *)0)->member) ||	\
		      __same_type(*(ptr), void),			\
		      "pointer type mismatch in container_of()");	\
	((type *)(__mptr - offsetof(type, member))); })

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

const char *getTaskName() {
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
    const char* taskName = (currentTask != NULL) ? pcTaskGetName(currentTask) : "NotInTask";

    return taskName;
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

    if (size == 0) {
        msg->data = NULL;
        msg->len = 0;
        return msg;
    }

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

void MessageMove(Message **dest,Message **src) {
    assert(dest && src);
    assert(*dest == NULL && *src != NULL);

    *dest = *src;
    *src = NULL;
    return;
}

// ================================================
int echo_handler(Message *input, Message *output) {
    if (!input || !output || !input->data || !output->data) {
        printf("[echo_handler] empty input->data, ret -1200\n");
        return -1200;
    }
    int output_len = output->len;
    output->len = MessageCopy(output, 0, input, 0, input->len);
    printf("[echo_handler] input->len: %lu, output->len: %lu -> %lu\n", (unsigned long)input->len, (unsigned long)output_len, (unsigned long)output->len);

    return 0;
}

// ================================================
typedef struct {
    Message *input;
    Message *output;
    Func     func;
} RpcRequest;

int rpc_call(Func func, Message *input, Message *output) {
    RpcRequest req = {
        .input  = input,
        .output = output,
        .func   = func,
    };
    int ret;

    xQueueSend(xRequestQueue, &req, portMAX_DELAY);
    xQueueReceive(xResponseQueue, &ret, portMAX_DELAY);
    return ret;
}

void rpc_dispatch(void) {
    RpcRequest req;

    xQueueReceive(xRequestQueue, &req, portMAX_DELAY);

    // printf("[rpc_dispatch] Processing: len=%lu, first2byte=[%02X %02X]\n",
    //        req.input->len,
    //        (unsigned char)req.input->data[0],
    //        (unsigned char)req.input->data[1]);

    int ret = req.func(req.input, req.output);

    // printf("[rpc_dispatch] RPC done, ret=%d, reply back\n", ret);
    xQueueSend(xResponseQueue, &ret, portMAX_DELAY);
}

// ================================================
// multi RPC extension
// ================================================

#define MULTI_CAP 128
#define HANDLER_RET_SIZE 4
#define MULTI_REPLY_UNKNOWN UINT32_MAX  // sentinel: server hasn't determined reply length yet

// ================================================
// --- Coroutine primitives (Duff's device style) ---
// ================================================

struct ctx_t;
typedef void (*CtxResetFunc)(void *ctx);

typedef struct ctx_t {
    int line;
    CtxResetFunc reset;
} Ctx;

#define CR_FIELD \
    int line; \
    CtxResetFunc reset


#define CR_START(ctx)     switch((ctx)->line) { case 0:
#define CR_START_UNTIL(ctx, cond) switch((ctx)->line) { case 0: if(!(cond)) { ctx->line = 0; return 0; }
#define CR_IS_STARTED(ctx) ((ctx)->line != 0)

#define CR_YIELD(ctx) do { (ctx)->line = __LINE__; return (0); case __LINE__:; } while(0)
#define CR_END(ctx)       } (ctx)->line = 0;

#define CR_AWAIT(ctx, cond) \
    while(!(cond)) { CR_YIELD(ctx); }

#define CR_RESET(ctx, ret)     do { (ctx)->line = 0; if((ctx)->reset) ((ctx)->reset)(ctx); return ret; } while(0)
// #define CR_RESET_IF(ctx, cond, ret)      do { if (cond)    { CR_RESET(ctx, ret); } } while (0)
// #define CR_RESET_NOT_IF(ctx, cond, ret)  do { if (!(cond)) { CR_RESET(ctx, ret); } } while (0)
// #define CR_RESET_ASSERT(ctx, cond, ret)  CR_RESET_NOT_IF(ctx, cond, ret)

// ================================================
// --- Protocol headers ---
// ================================================
#define MULTI_MAGIC 0xDEC0

/* multi protocol error codes: negative = protocol error, 0 = OK, positive = handler error */
#define MULTI_OK              0

#define MULTI_ERR_BASE     -1100
#define MULTI_ERR_MAGIC     (MULTI_ERR_BASE - 1)  // send hdr magic mismatch
#define MULTI_ERR_SHORT     (MULTI_ERR_BASE - 2)  // input too short for SendHdr
#define MULTI_ERR_OVERSIZE  (MULTI_ERR_BASE - 3)  // total > rx capacity
#define MULTI_ERR_NOMEM     (MULTI_ERR_BASE - 4)  // buffer allocation failed
#define MULTI_ERR_OUTOFSYNC     (MULTI_ERR_BASE - 5)
                                                  //
#define MULTI_ERR_RESP_HDR  (MULTI_ERR_BASE - 20)  // response too short for RecvHdr
#define MULTI_ERR_RESP_MAGIC (MULTI_ERR_BASE - 21) // response magic mismatch
#define MULTI_ERR_RESP_OVERSIZE (MULTI_ERR_BASE - 22) // response payload exceeds output buffer
#define MULTI_ERR_RESP_UPDATE_ERR  (MULTI_ERR_BASE - 23)  // response update error
#define MULTI_ERR_RESP_DONE_ERR  (MULTI_ERR_BASE - 24)  // response done error


typedef enum {
    SendInvalid = 0,
    SendStart   = 1,
    SendUpdate  = 2,
    SendDone    = 3,
} SendType;

typedef struct {
    SendType  type;
    union {
        struct { uint32_t total;  } start;
        struct { uint32_t offset; } update;
        struct {} done;
    } as;
} SendHdr;

typedef enum {
    RespInvalid = 0,
    RespOK      = 1,
    RespErr     = 2,
} RespType;

typedef struct {
    RespType  type;
    union {
        struct {} ok;
        struct { int errcode; } err;
    } as;
} RespHdr;

typedef struct {
    uint16_t magic;
    SendHdr  sh;
    RespHdr  rh;
} Hdr;

// ================================================
// Protocol logging
// ================================================
static void logHdr(const char *prefix, Hdr *hdr) {
    const char *st_str = "Inv";
    switch (hdr->sh.type) {
        case SendStart:  st_str = "Sta"; break;
        case SendUpdate: st_str = "Upd"; break;
        case SendDone:   st_str = "Don"; break;
    }
    const char *rt_str = "Inv";
    switch (hdr->rh.type) {
        case RespOK:  rt_str = "OK "; break;
        case RespErr: rt_str = "Err"; break;
    }

    if (hdr->sh.type == SendStart) {
        printf("[%s] %s: sh=%s(%lu) rh=%s\n",
               getTaskName(), prefix, st_str,
               (unsigned long)hdr->sh.as.start.total, rt_str);
    } else if (hdr->sh.type == SendUpdate) {
        printf("[%s] %s: sh=%s(%lu) rh=%s\n",
               getTaskName(), prefix, st_str,
               (unsigned long)hdr->sh.as.update.offset, rt_str);
    } else {
        printf("[%s] %s: sh=%s rh=%s\n",
               getTaskName(), prefix, st_str, rt_str);
    }
}

// ================================================

typedef struct {
    CR_FIELD;
    Message *rx;
    uint32_t max_total;
} RecvCtx;

typedef struct {
    CR_FIELD;
    Message *tx;
    uint32_t pos;
    uint32_t ret;

} SendCtx;

typedef struct {
    CR_FIELD;
    Message *in;
} HandlerCtx;

typedef struct {
    RecvCtx recv_ctx;
    SendCtx send_ctx;

    HandlerCtx handler_ctx;
    int error;
} MultiSession;

typedef struct data_t  {
    // input
    Message *input;
    Hdr *ihdr;
    char *idata;
    uint32_t ilen;

    // output
    Message *output;
    Hdr *ohdr;
    char *odata;
    uint32_t olen;

} Data;

// ================================================
// Ctx reset helpers
// ================================================
static void RecvCtxReset(void *vctx) {
    RecvCtx *ctx = (RecvCtx *)vctx;
    if(ctx->rx) {
        MessageDelete(ctx->rx);
        ctx->rx = NULL;
    }
}

static void SendCtxResetNoFree(void *vctx) {
    SendCtx *ctx = (SendCtx *)vctx;
    ctx->tx = NULL;
    ctx->pos = 0;
    ctx->ret = 0;
}

static void SendCtxResetFree(void *vctx) {
    SendCtx *ctx = (SendCtx *)vctx;
    if (ctx->tx) {
        MessageDelete(ctx->tx);
        ctx->tx = NULL;
    }
    ctx->pos = 0;
    ctx->ret = 0;
}

// ================================================
// Protocol Implment
// ================================================
static int StepMultiSession(MultiSession *s, Message *input, Message *output) {
    if (!input || !output) {
        return MULTI_ERR_SHORT;
    }
    if (input->len < sizeof(Hdr) || output->len < sizeof(Hdr)) {
        return MULTI_ERR_SHORT;
    }

    Data data = {
        .input = input,
        .ihdr = (Hdr *)input->data,
        .idata = input->data + sizeof(Hdr),
        .ilen = input->len - sizeof(Hdr),

        .output = output,
        .ohdr = (Hdr *)output->data,
        .odata = output->data + sizeof(Hdr),
        .olen = output->len - sizeof(Hdr),
    };

    if (data.ihdr->magic != MULTI_MAGIC) return MULTI_ERR_MAGIC;

    data.ohdr->magic = MULTI_MAGIC;
    data.ohdr->sh.type = SendInvalid;
    data.ohdr->rh.type = RespInvalid;

    logHdr("IN ", data.ihdr);

    int CoRecv(RecvCtx *ctx, Data *data);
    CoRecv(&s->recv_ctx, &data);

    if (strcmp(getTaskName(), "TaskServer") == 0) {
        // echo_handler()
        int CoHandler(HandlerCtx *ctx, Data *data);
        CoHandler(&s->handler_ctx, &data);
    }

    int CoSend(SendCtx *ctx, Data *data);
    CoSend(&s->send_ctx, &data);

    // fallbcak, if CoSend not being run, we
    if ( data.ohdr->sh.type == SendInvalid) {
        data.output->len = sizeof(Hdr);
    }

    logHdr("OUT", data.ohdr);
    return 0;
}

// ================================================
// CoRecv
// ================================================
void resp(Data *data, int code) {
    if (code == 0) {
        data->ohdr->rh.type = RespOK;
    } else {
        data->ohdr->rh.type = RespErr;
        data->ohdr->rh.as.err.errcode = code;
    }
}


int CoRecv(RecvCtx *ctx, Data *data) {
    Hdr *ihdr  = data->ihdr;

    uint32_t ilen = data->ilen;

    char* idata = data->idata;


    CR_START_UNTIL(ctx, ihdr->sh.type == SendStart);


    // Start Stage
    if (ihdr->sh.as.start.total > ctx->max_total) {
        resp(data, MULTI_ERR_OVERSIZE);
        CONTAINER_OF(ctx, MultiSession, recv_ctx)->error = MULTI_ERR_OVERSIZE;
        RecvCtxReset(ctx);
        CR_RESET(ctx, 0);
    }

    RecvCtxReset(ctx);
    ctx->rx = MessageCreate(ihdr->sh.as.start.total);
    if (ctx->rx == NULL) {
        resp(data, MULTI_ERR_NOMEM);
        CONTAINER_OF(ctx, MultiSession, recv_ctx)->error = MULTI_ERR_NOMEM;
        RecvCtxReset(ctx);
        CR_RESET(ctx, 0);
    }
    memset(ctx->rx->data, 0, ctx->rx->len);

    // Update / Done Stage
    while(1) {
        resp(data, MULTI_OK);
        CR_YIELD(ctx);
        if(ihdr->sh.type != SendUpdate && ihdr->sh.type != SendDone) {
            resp(data, MULTI_ERR_OUTOFSYNC);
            CONTAINER_OF(ctx, MultiSession, recv_ctx)->error = MULTI_ERR_OUTOFSYNC;
            CR_RESET(ctx, 0);
        }

        if (ihdr->sh.type == SendDone) {
            break;
        }
        if(!(ihdr->sh.as.update.offset + ilen <= ctx->rx->len)) {
            resp(data, MULTI_ERR_OVERSIZE);
            CONTAINER_OF(ctx, MultiSession, recv_ctx)->error = MULTI_ERR_OVERSIZE;
            RecvCtxReset(ctx);
            CR_RESET(ctx, 0);
        }
        memcpy(ctx->rx->data + ihdr->sh.as.update.offset, idata, ilen);
    }

    MessageMove(&CONTAINER_OF(ctx, MultiSession, recv_ctx)->handler_ctx.in, &ctx->rx);

    resp(data, MULTI_OK);
    RecvCtxReset(ctx);
    CR_RESET(ctx, 0);

    CR_END(ctx);
    return 0;
}

// ================================================
// CoHandler
// ================================================
int CoHandler(HandlerCtx *ctx, Data *data) {
    CR_START_UNTIL(ctx, ctx->in != NULL);

    Message *out = MessageCreate(MULTI_CAP + HANDLER_RET_SIZE);

    Message handler_input = {
        .data = ctx->in->data,
        .len  = ctx->in->len,
    };
    Message handler_output = {
        .data = out->data + HANDLER_RET_SIZE,
        .len  = (out->len > HANDLER_RET_SIZE) ? out->len - HANDLER_RET_SIZE : 0,
    };
    int ret = echo_handler(&handler_input, &handler_output);

    *(int32_t *)(out->data) = ret;
    out->len = HANDLER_RET_SIZE + handler_output.len;

    MessageDelete(ctx->in);
    ctx->in = NULL;

    MessageMove(&CONTAINER_OF(ctx, MultiSession, handler_ctx)->send_ctx.tx, &out);
    CR_END(ctx);
    return 0;

}




// ================================================
// CoSend
// ================================================
void send_start(Data *data, int total) {
    data->ohdr->sh.type = SendStart;
    data->ohdr->sh.as.start.total = total;
    data->output->len = sizeof(Hdr);
}

void send_update(Data *data, int offset, char *src, uint32_t len) {
    data->ohdr->sh.type = SendUpdate;
    data->ohdr->sh.as.update.offset = offset;
    memcpy(data->odata, src, len);
    data->output->len = sizeof(Hdr) + len;
}


void send_done(Data *data) {
    data->ohdr->sh.type = SendDone;
    data->output->len = sizeof(Hdr);
}

int CoSend(SendCtx *ctx, Data *data) {
    Hdr *ihdr  = data->ihdr;
    uint32_t olen = data->olen;

    CR_START_UNTIL(ctx, ctx->tx != NULL); // ctx->tx are manage outside of ctx

    // Start
    ctx->pos = 0;
    ctx->ret = 0;

    send_start(data, ctx->tx->len);
    CR_YIELD(ctx);
    if (ihdr->rh.type != RespOK) {
        printf("[%s] S: Start error %d\n", getTaskName(), ihdr->rh.as.err.errcode);
        CONTAINER_OF(ctx, MultiSession, send_ctx)->error = ihdr->rh.as.err.errcode;
        CR_RESET(ctx, 0);
    }


    int chunk = 0;

    while(1) {
        // Update
        chunk = ctx->tx->len - ctx->pos > olen ? olen : ctx->tx->len - ctx->pos;
        if (chunk <= 0) {
            break;
        }

        send_update(data, ctx->pos, ctx->tx->data + ctx->pos, chunk);
        ctx->pos += chunk;
        CR_YIELD(ctx);
        if (ihdr->rh.type != RespOK) {
            printf("[%s] S: Update error %d\n", getTaskName(), ihdr->rh.as.err.errcode);
            CONTAINER_OF(ctx, MultiSession, send_ctx)->error = ihdr->rh.as.err.errcode;
            CR_RESET(ctx, 0);
        }
    }


    send_done(data);
    CR_YIELD(ctx);
    if (ihdr->rh.type != RespOK) {
        printf("[%s] S: Done error %d\n", getTaskName(), ihdr->rh.as.err.errcode);
        CONTAINER_OF(ctx, MultiSession, send_ctx)->error = ihdr->rh.as.err.errcode;
        CR_RESET(ctx, 0);
    }

    CR_RESET(ctx, 0);

    CR_END(ctx);
    return 0;

}

// ================================================
// Adapter layer for task
// ================================================

// --- Public server entry ---
int multi_handler(Message *input, Message *output)
{
    static MultiSession srv_session;

    if (srv_session.send_ctx.reset == NULL) {
        srv_session.send_ctx.reset = SendCtxResetFree;
        srv_session.recv_ctx.reset = RecvCtxReset;
        srv_session.recv_ctx.max_total = MULTI_CAP;
    }

    StepMultiSession(&srv_session, input, output);

    return 0;
}

// --- Client: symmetric multi RPC ---
void SetMockInput(Message *message) {
    Hdr *hdr = (Hdr *)message->data;
    hdr->magic = MULTI_MAGIC;
    hdr->sh.type = SendInvalid;
    hdr->rh.type = RespInvalid;
    message->len = sizeof(Hdr);
}



int rpc_call_multi(Func func, Message *input, Message *output)
{
    MultiSession session = {0};

    session.send_ctx.tx = input;
    session.send_ctx.reset = SendCtxResetNoFree;
    session.recv_ctx.reset = RecvCtxReset;
    session.recv_ctx.max_total = MULTI_CAP + HANDLER_RET_SIZE;

    char swap_ibuffer[MESSAGE_MAX_LEN] = {0};
    char swap_obuffer[MESSAGE_MAX_LEN] = {0};


    Message swap_imessage = {
        .len = sizeof(swap_ibuffer),
        .data = swap_ibuffer,
    };
    Message swap_omessage = {
        .len = sizeof(swap_obuffer),
        .data = swap_obuffer,
    };
    SetMockInput(&swap_imessage); // mock input for first time to kick start

    int round = 0;
    do {
        round++;
        printf("[%s] ===== ROUND %d =====\n", getTaskName(), round);

        swap_omessage.len = sizeof(swap_obuffer);
        swap_omessage.data = swap_obuffer;
        int step_ret = StepMultiSession(&session, &swap_imessage, &swap_omessage);
        if (step_ret != 0) {
            session.error = step_ret;
        }

        swap_imessage.len = sizeof(swap_ibuffer);
        swap_imessage.data = swap_ibuffer;
        rpc_call(multi_handler, &swap_omessage, &swap_imessage);

    } while(CR_IS_STARTED(&session.send_ctx) || CR_IS_STARTED(&session.recv_ctx));

    if (session.error != 0) {
        return session.error;
    }

    if (session.handler_ctx.in != NULL) {
        if (session.handler_ctx.in->len < HANDLER_RET_SIZE) {
            MessageDelete(session.handler_ctx.in);
            session.handler_ctx.in = NULL;
            return 0;
        }

        int32_t handler_ret = *(int32_t *)session.handler_ctx.in->data;
        uint32_t payload_len = session.handler_ctx.in->len - HANDLER_RET_SIZE;

        if (handler_ret != 0) {
            MessageDelete(session.handler_ctx.in);
            session.handler_ctx.in = NULL;
            return handler_ret;
        }

        if (payload_len > output->len) {
            MessageDelete(session.handler_ctx.in);
            session.handler_ctx.in = NULL;
            return MULTI_ERR_RESP_OVERSIZE;
        }

        output->len = MessageCopy(output, 0, session.handler_ctx.in, HANDLER_RET_SIZE, payload_len);
        MessageDelete(session.handler_ctx.in);
        session.handler_ctx.in = NULL;
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

static int test_echo(uint32_t input_len, uint32_t output_len, uint32_t seed) {
    printf("============================================\n");
    printf("test_echo: input %u -> output %u, sedd %u\n", input_len, output_len, seed);
    printf("============================================\n");
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

void TaskClient(void *pvParameters) {
    (void)pvParameters;
    uint32_t counter = 0;

    printHeapAndStack(__LINE__);
    for (;;) {
        // printHeapAndStack(__LINE__);

        int ret;

        /* 100-byte payload (table verification) */
        ret = test_echo(100, 128, 999);
        printf("[TaskClient] test 100: ret=%d (expect 0)\n\n", ret);

        /* 0-byte payload */
        ret = test_echo(0, MULTI_CAP, counter);
        printf("[TaskClient] test 0: ret=%d (expect -1200)\n\n", ret);

        /* MULTI_CAP - 1 */
        ret = test_echo(MULTI_CAP - 1, MULTI_CAP, counter);
        printf("[TaskClient] test MULTI_CAP-1: ret=%d (expect 0)\n\n", ret);

        /* MULTI_CAP */
        ret = test_echo(MULTI_CAP, MULTI_CAP, counter);
        printf("[TaskClient] test MULTI_CAP: ret=%d (expect 0)\n\n", ret);

        /* MULTI_CAP + 1 -> server rejects as oversize */
        ret = test_echo(MULTI_CAP + 1, MULTI_CAP, counter);
        printf("[TaskClient] test MULTI_CAP+1: ret=%d (expect %d)\n\n",
               ret, MULTI_ERR_OVERSIZE);

        /* output buffer too small -> client rejects as resp oversize */
        ret = test_echo(MULTI_CAP, MULTI_CAP / 2, counter);
        printf("[TaskClient] test out=512: ret=%d (expect %d)\n\n",
               ret, MULTI_ERR_RESP_OVERSIZE);


        counter++;
        vTaskDelay(pdMS_TO_TICKS(1000));

        while(1);

        // printHeapAndStack(__LINE__);
    }
}

void TaskServer(void *pvParameters) {
    (void)pvParameters;
    for (;;) {
        rpc_dispatch();
    }
}

int main() {
    setup();
    printf("Init complete\n");

    xRequestQueue  = xQueueCreate(QUEUE_LENGTH, sizeof(RpcRequest));
    xResponseQueue = xQueueCreate(QUEUE_LENGTH, sizeof(int));
    if (xRequestQueue == NULL || xResponseQueue == NULL) {
        printf("Queue creation failed!\n");
        while (1);
    }

    printHeapAndStack(__LINE__);
    xTaskCreate(TaskClient,
                "TaskClient",
                configMINIMAL_STACK_SIZE * 8,
                NULL,
                (tskIDLE_PRIORITY + 1),
                NULL);
    xTaskCreate(TaskServer,
                "TaskServer",
                configMINIMAL_STACK_SIZE * 4,
                NULL,
                (tskIDLE_PRIORITY + 1),
                NULL);

    vTaskStartScheduler();
    while (1);
}
