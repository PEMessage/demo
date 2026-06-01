#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>
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

typedef struct ctx_t {
    int line;
} Ctx;

#define CR_FIELD \
    int line

#define CR_FIELD_WITH_ERROR \
    int line; \
    int error

#define CR_START(ctx)     switch((ctx)->line) { case 0:
#define CR_START_UNTIL(ctx, cond) switch((ctx)->line) { case 0: if(!(cond)) { ctx->line = 0; return 0; }
#define CR_IS_STARTED(ctx) ((ctx)->line != 0)

#define CR_YIELD(ctx) do { (ctx)->line = __LINE__; return (0); case __LINE__:; } while(0)
#define CR_END(ctx)       } (ctx)->line = 0;

#define CR_AWAIT(ctx, cond) \
    while(!(cond)) { CR_YIELD(ctx); }

#define CR_RESET(ctx)     do { (ctx)->line = 0; return 0; } while(0)

#define CR_RESET_WITH(ctx, err) do { (ctx)->line = 0; (ctx)->error = (err); return 0; } while(0)

// ================================================
// --- Protocol headers ---
// ================================================
#define MULTI_MAGIC 0xDEC0

/* multi protocol error codes: negative = protocol error, 0 = OK, positive = handler error */
#define MULTI_OK              0

/* General / StepMultiSession errors: -1100 range */
#define MULTI_ERR_BASE         -1100
#define MULTI_ERR_MAGIC        (MULTI_ERR_BASE - 1)   /* magic mismatch */
#define MULTI_ERR_SHORT        (MULTI_ERR_BASE - 2)   /* buffer too short for Hdr */
#define MULTI_ERR_NULLINPUT    (MULTI_ERR_BASE - 3)   /* NULL input/output */

/* CoRecv errors: -1200 range */
#define MULTI_ERR_CORECV_BASE   -1200
#define MULTI_ERR_CORECV_NOMEM       (MULTI_ERR_CORECV_BASE - 1)  /* buffer allocation failed */
#define MULTI_ERR_CORECV_OUTOFSYNC   (MULTI_ERR_CORECV_BASE - 2)  /* unexpected send type */
#define MULTI_ERR_CORECV_OVERSIZE    (MULTI_ERR_CORECV_BASE - 3)  /* offset+len > rx capacity */
#define MULTI_ERR_CORECV_OFFSET      (MULTI_ERR_CORECV_BASE - 4)  /* offset != pos */
#define MULTI_ERR_CORECV_INCOMPLETE  (MULTI_ERR_CORECV_BASE - 5)  /* pos != total at Done */

/* CoSend errors: -1300 range */
#define MULTI_ERR_COSEND_BASE   -1300
#define MULTI_ERR_COSEND_START_ERR   (MULTI_ERR_COSEND_BASE - 1)  /* remote rejected start */
#define MULTI_ERR_COSEND_UPDATE_ERR  (MULTI_ERR_COSEND_BASE - 2)  /* remote rejected update */
#define MULTI_ERR_COSEND_DONE_ERR    (MULTI_ERR_COSEND_BASE - 3)  /* remote rejected done */

/* ServerEvent (OnEventServer) errors: -1400 range */
#define MULTI_ERR_ONEVENT_SERVER_BASE   -1400
#define MULTI_ERR_ONEVENT_SERVER_OVERSIZE    (MULTI_ERR_ONEVENT_SERVER_BASE - 1)  /* total > CLIENT_MAX_CAP */
#define MULTI_ERR_ONEVENT_SERVER_NOMEM       (MULTI_ERR_ONEVENT_SERVER_BASE - 2)  /* MessageCreate failed */
#define MULTI_ERR_ONEVENT_SERVER_RECV_ERR    (MULTI_ERR_ONEVENT_SERVER_BASE - 3)  /* recv not done */
#define MULTI_ERR_ONEVENT_SERVER_SEND_ERR    (MULTI_ERR_ONEVENT_SERVER_BASE - 4)  /* send not start */
#define MULTI_ERR_ONEVENT_SERVER_SEND_DONE_ERR (MULTI_ERR_ONEVENT_SERVER_BASE - 5) /* send not done */

/* ClientEvent (OnEventClient) errors: -1500 range */
#define MULTI_ERR_ONEVENT_CLIENT_BASE   -1500
#define MULTI_ERR_ONEVENT_CLIENT_SEND_ERR      (MULTI_ERR_ONEVENT_CLIENT_BASE - 1)  /* send not start */
#define MULTI_ERR_ONEVENT_CLIENT_SEND_DONE_ERR (MULTI_ERR_ONEVENT_CLIENT_BASE - 2)  /* send not done */
#define MULTI_ERR_ONEVENT_CLIENT_RECV_ERR      (MULTI_ERR_ONEVENT_CLIENT_BASE - 3)  /* recv not start */
#define MULTI_ERR_ONEVENT_CLIENT_RECV_OVERSIZE (MULTI_ERR_ONEVENT_CLIENT_BASE - 4)  /* recv total > rx_init->len */
#define MULTI_ERR_ONEVENT_CLIENT_FMT_UNKNOWN   (MULTI_ERR_ONEVENT_CLIENT_BASE - 5)  /* rx_done == NULL */
#define MULTI_ERR_ONEVENT_CLIENT_FMT_TOOSHORT  (MULTI_ERR_ONEVENT_CLIENT_BASE - 6)  /* rx->len < HANDLER_RET_SIZE */
#define MULTI_ERR_ONEVENT_CLIENT_FMT_TOOLONG   (MULTI_ERR_ONEVENT_CLIENT_BASE - 7)  /* rx_view.len > expected */


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
    char sh_buf[32];
    switch (hdr->sh.type) {
        case SendStart:
            snprintf(sh_buf, sizeof(sh_buf), "Start(%lu)", (unsigned long)hdr->sh.as.start.total);
            break;
        case SendUpdate:
            snprintf(sh_buf, sizeof(sh_buf), "Update(%lu)", (unsigned long)hdr->sh.as.update.offset);
            break;
        case SendDone:
            snprintf(sh_buf, sizeof(sh_buf), "Done");
            break;
        default:
            snprintf(sh_buf, sizeof(sh_buf), "Invalid");
            break;
    }

    char rh_buf[32];
    switch (hdr->rh.type) {
        case RespOK:
            snprintf(rh_buf, sizeof(rh_buf), "OK");
            break;
        case RespErr:
            snprintf(rh_buf, sizeof(rh_buf), "Err(%d)", hdr->rh.as.err.errcode);
            break;
        default:
            snprintf(rh_buf, sizeof(rh_buf), "Invalid");
            break;
    }

    printf("[%s] %s: sh=%s rh=%s\n",
           getTaskName(), prefix, sh_buf, rh_buf);
}

// ================================================

typedef struct {
    CR_FIELD_WITH_ERROR;
    Message *rx;
    uint32_t pos;
} RecvCtx;

typedef struct {
    CR_FIELD_WITH_ERROR;
    Message *tx;
    uint32_t pos;

} SendCtx;

typedef struct {
    CR_FIELD_WITH_ERROR;
    Message *rx;
    Message *tx;
} ServerEventCtx;

typedef struct {
    CR_FIELD_WITH_ERROR;

    // recv side
    // raw buffer for recv
    Message *rx_init;

    // if rx done, rx_done != NULL
    Message *rx_done;
    // if rx_done exist, following field is valid
    Message rx_view;
    int handler_ret;


    // send side
    Message *tx;
    Func handler;
    // raw buffer for send
    Message *tx_init;
} ClientEventCtx;

typedef struct {
    ClientEventCtx client;
    ServerEventCtx server;
} EventCtx;

typedef struct {
    RecvCtx recv_ctx;
    SendCtx send_ctx;

    EventCtx event_ctx;

    uint32_t round;
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



typedef enum {
    EvPreOnFirstStep,
    EvPreSendStart,

    EvRecvStart,
    EvRecvErr,
    EvRecvDone,

    EvSendStart,
    EvSendErr,
    EvSendDone,
} EventType;

typedef struct {
    EventType type;
    union {
        struct {} pre_on_first_step;
        struct {} pre_recv_start;

        struct {uint32_t total;} recv_start;
        struct {} recv_err;
        struct {} recv_done;

        struct {} send_start;
        struct {} send_err;
        struct {} send_done;
    } as;
} Event;


// ================================================
// Protocol Implment
// ================================================
// Invariant:
//  if session state might change, this function should always return 0.
//  if session state not change for sure, could return other value
static int StepMultiSession(MultiSession *s, Message *input, Message *output) {
    s->round++;
    if (!input || !output) {
        printf("[%s] ERR: StepMultiSession: NULL input/output\n", getTaskName());
        return MULTI_ERR_NULLINPUT;
    }
    if (input->len < sizeof(Hdr) || output->len < sizeof(Hdr)) {
        printf("[%s] ERR: StepMultiSession: buffer too short (input=%lu, output=%lu, need=%zu)\n",
               getTaskName(), (unsigned long)input->len, (unsigned long)output->len, sizeof(Hdr));
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

    if (data.ihdr->magic != MULTI_MAGIC) {
        printf("[%s] ERR: StepMultiSession: magic mismatch (got=0x%04X, want=0x%04X)\n",
               getTaskName(), (unsigned)data.ihdr->magic, (unsigned)MULTI_MAGIC);
        return MULTI_ERR_MAGIC;
    }

    data.ohdr->magic = MULTI_MAGIC;
    data.ohdr->sh.type = SendInvalid;
    data.ohdr->rh.type = RespInvalid;

    logHdr("IN ", data.ihdr);

    int PreEvent(MultiSession *s, Data *data);
    PreEvent(s, &data);

    int CoRecv(RecvCtx *ctx, Data *data);
    CoRecv(&s->recv_ctx, &data);

    int CoSend(SendCtx *ctx, Data *data);
    CoSend(&s->send_ctx, &data);

    // fallbcak, if CoSend not being run, force set some return value
    if (data.ohdr->sh.type == SendInvalid) {
        data.output->len = sizeof(Hdr);
    }

    logHdr("OUT", data.ohdr);
    return 0;
}

void OnEvent(EventCtx *ctx, Event *ev);


// ================================================
// CoRecv
// ================================================
// Invariant: CoRecv should not alloc any memory
void resp(Data *data, int code) {
    if (code == 0) {
        data->ohdr->rh.type = RespOK;
    } else {
        data->ohdr->rh.type = RespErr;
        data->ohdr->rh.as.err.errcode = code;
    }
}

static void OnRecvStart(EventCtx *ctx, uint32_t total) {
    Event ev = {
        .type = EvRecvStart,
        .as.recv_start.total = total,
    };
    OnEvent(ctx, &ev);

}

static void OnRecvErr(EventCtx *ctx) {
    Event ev = {
        .type = EvRecvErr,
    };
    OnEvent(ctx, &ev);
}

static void OnRecvDone(EventCtx *ctx) {
    Event ev = {
        .type = EvRecvDone,
    };
    OnEvent(ctx, &ev);
}

static void RecvReset(RecvCtx *recv) {
    recv->rx = NULL;
    recv->pos = 0;
}

int CoRecv(RecvCtx *ctx, Data *data) {
    Hdr *ihdr  = data->ihdr;

    uint32_t ilen = data->ilen;

    char* idata = data->idata;
    MultiSession *s = CONTAINER_OF(ctx, MultiSession, recv_ctx);


    CR_START_UNTIL(ctx, ihdr->sh.type == SendStart);

    RecvReset(ctx);
    OnRecvStart(&s->event_ctx, ihdr->sh.as.start.total);
    if (ctx->rx == NULL) {
        printf("[%s] ERR: CoRecv: rx buffer not allocated (NOMEM)\n", getTaskName());
        resp(data, MULTI_ERR_CORECV_NOMEM);
        OnRecvErr(&s->event_ctx);

        RecvReset(ctx);
        CR_RESET_WITH(ctx, MULTI_ERR_CORECV_NOMEM);
    }
    memset(ctx->rx->data, 0, ctx->rx->len);
    ctx->pos = 0;

    // Update / Done Stage
    while(1) {
        resp(data, MULTI_OK);
        CR_YIELD(ctx);
        if(ihdr->sh.type != SendUpdate && ihdr->sh.type != SendDone) {
            printf("[%s] ERR: CoRecv: out of sync (sh.type=%d, expected Update/Done)\n",
                   getTaskName(), (int)ihdr->sh.type);
            resp(data, MULTI_ERR_CORECV_OUTOFSYNC);
            OnRecvErr(&s->event_ctx);

            CR_RESET_WITH(ctx, MULTI_ERR_CORECV_OUTOFSYNC);
        }

        if (ihdr->sh.type == SendDone) {
            break;
        }
        if (ihdr->sh.as.update.offset != ctx->pos) {
            printf("[%s] ERR: CoRecv: offset mismatch (offset=%lu, pos=%lu)\n",
                   getTaskName(), (unsigned long)ihdr->sh.as.update.offset,
                   (unsigned long)ctx->pos);
            resp(data, MULTI_ERR_CORECV_OFFSET);
            OnRecvErr(&s->event_ctx);

            RecvReset(ctx);
            CR_RESET_WITH(ctx, MULTI_ERR_CORECV_OFFSET);
        }
        if(!(ihdr->sh.as.update.offset + ilen <= ctx->rx->len)) {
            printf("[%s] ERR: CoRecv: oversize (offset=%lu, ilen=%lu, rx_len=%lu)\n",
                   getTaskName(), (unsigned long)ihdr->sh.as.update.offset,
                   (unsigned long)ilen, (unsigned long)ctx->rx->len);
            resp(data, MULTI_ERR_CORECV_OVERSIZE);
            OnRecvErr(&s->event_ctx);

            RecvReset(ctx);
            CR_RESET_WITH(ctx, MULTI_ERR_CORECV_OVERSIZE);
        }
        memcpy(ctx->rx->data + ihdr->sh.as.update.offset, idata, ilen);
        ctx->pos += ilen;
    }

    if (ctx->pos != ctx->rx->len) {
        printf("[%s] ERR: CoRecv: incomplete (pos=%lu, total=%lu)\n",
               getTaskName(), (unsigned long)ctx->pos, (unsigned long)ctx->rx->len);
        resp(data, MULTI_ERR_CORECV_INCOMPLETE);
        OnRecvErr(&s->event_ctx);

        RecvReset(ctx);
        CR_RESET_WITH(ctx, MULTI_ERR_CORECV_INCOMPLETE);
    }

    resp(data, MULTI_OK);
    OnRecvDone(&s->event_ctx);

    RecvReset(ctx);
    CR_RESET(ctx);

    CR_END(ctx);
    return 0;
}


// ================================================
// CoSend
// ================================================
// Invariant: CoSend should not alloc any memory
void send_start(Data *data, uint32_t total) {
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

static void OnSendStart(EventCtx *ctx) {
    Event ev = {
        .type = EvSendStart,
    };
    OnEvent(ctx, &ev);
}

static void OnSendErr(EventCtx *ctx) {
    Event ev = {
        .type = EvSendErr,
    };
    OnEvent(ctx, &ev);
}

static void OnSendDone(EventCtx *ctx) {
    Event ev = {
        .type = EvSendDone,
    };
    OnEvent(ctx, &ev);
}

static void SendReset(SendCtx *send) {
    send->pos = 0;
    send->tx = NULL;
}

int CoSend(SendCtx *ctx, Data *data) {
    Hdr *ihdr  = data->ihdr;
    uint32_t olen = data->olen;
    MultiSession *s = CONTAINER_OF(ctx, MultiSession, send_ctx);

    CR_START_UNTIL(ctx, ctx->tx != NULL); // ctx->tx are manage outside of ctx

    send_start(data, ctx->tx->len);
    OnSendStart(&s->event_ctx);
    CR_YIELD(ctx);
    if (ihdr->rh.type != RespOK) {
        OnSendErr(&s->event_ctx);

        printf("[%s] ERR: CoSend: remote rejected start (rh.err=%d)\n", getTaskName(), ihdr->rh.as.err.errcode);

        SendReset(ctx);
        CR_RESET_WITH(ctx, MULTI_ERR_COSEND_START_ERR);
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
            OnSendErr(&s->event_ctx);

            printf("[%s] ERR: CoSend: remote rejected update (rh.err=%d)\n", getTaskName(), ihdr->rh.as.err.errcode);

            SendReset(ctx);
            CR_RESET_WITH(ctx, MULTI_ERR_COSEND_UPDATE_ERR);
        }
    }

    send_done(data);
    OnSendDone(&s->event_ctx);

    CR_YIELD(ctx);
    if (ihdr->rh.type != RespOK) {

        printf("[%s] ERR: CoSend: remote rejected done (rh.err=%d)\n", getTaskName(), ihdr->rh.as.err.errcode);

        SendReset(ctx);
        CR_RESET_WITH(ctx, MULTI_ERR_COSEND_DONE_ERR);
    }

    SendReset(ctx);
    CR_RESET(ctx);

    CR_END(ctx);
    return 0;

}

// ================================================
// PreEvent(run before any other event per loop)
// ================================================
int PreEvent(MultiSession *s, Data *data) {
    if (s->round == 1) {
        Event ev = {
            .type = EvPreOnFirstStep
        };
        OnEvent(&s->event_ctx, &ev);
    }
    if (data->ihdr->sh.type == SendStart) {
        Event ev = {
            .type = EvPreSendStart
        };
        OnEvent(&s->event_ctx, &ev);
    }
    return 0;
}

// ================================================
// Adapter event handler for client and server
// ================================================

// It just a simplify of real world case
// In real-world case it will split to two Implment in 2 src file. compile to 2 binary.
//
// Since this is a PoC, and we only have one src file. we maually dispatch
void OnEvent(EventCtx *ctx, Event *ev) {
    MultiSession *s = CONTAINER_OF(ctx, MultiSession, event_ctx);
    if (strcmp(getTaskName(), "TaskClient") == 0) {
        int OnEventClient(ClientEventCtx *ctx, MultiSession *s, Event *ev);
        OnEventClient(&ctx->client, s, ev);
    } else {
        int OnEventServer(ServerEventCtx *ctx, MultiSession *s, Event *ev);
        OnEventServer(&ctx->server, s, ev);
    }
}



void ServerEventReset(ServerEventCtx *server) {
    MessageDelete(server->rx);
    MessageDelete(server->tx);

    server->rx = NULL;
    server->tx = NULL;
}

#define CLIENT_MAX_CAP MULTI_CAP
static_assert(MULTI_CAP >=  HANDLER_RET_SIZE, "Out of Sync");
int OnEventServer(ServerEventCtx *ctx, MultiSession *s, Event *ev) {

    // Some force reset way to allow recovery server.
    // if pervious session not finish for better Robustness
    //
    // logical this part don't have to exist.
    if (ev->type == EvPreOnFirstStep) {
        ServerEventReset(ctx);
        memset(&s->recv_ctx, 0, sizeof(s->recv_ctx));
        memset(&s->send_ctx, 0, sizeof(s->send_ctx));
        memset(&s->event_ctx, 0, sizeof(s->event_ctx));
    }

    if (ev->type == EvPreSendStart) {
        ServerEventReset(ctx);
    }

    CR_START_UNTIL(ctx, ev->type == EvRecvStart);
    ServerEventReset(ctx);

    if (ev->as.recv_start.total > CLIENT_MAX_CAP + HANDLER_RET_SIZE) {
        printf("[%s] ERR: OnEventServer: recv total exceeds cap (total=%lu, cap=%lu)\n",
               getTaskName(), (unsigned long)ev->as.recv_start.total,
               (unsigned long)(CLIENT_MAX_CAP + HANDLER_RET_SIZE));
        ServerEventReset(ctx);
        CR_RESET_WITH(ctx, MULTI_ERR_ONEVENT_SERVER_OVERSIZE);
    }

    ctx->rx = MessageCreate(ev->as.recv_start.total);
    if (!ctx->rx) {
        printf("[%s] ERR: OnEventServer: MessageCreate failed for rx (total=%lu)\n",
               getTaskName(), (unsigned long)ev->as.recv_start.total);
        s->recv_ctx.rx = NULL;
        ServerEventReset(ctx);
        CR_RESET_WITH(ctx, MULTI_ERR_ONEVENT_SERVER_NOMEM);
    }
    s->recv_ctx.rx = ctx->rx;

    CR_YIELD(ctx);
    if (ev->type != EvRecvDone) {
        printf("[%s] ERR: OnEventServer: expected EvRecvDone, got type=%d\n",
               getTaskName(), (int)ev->type);
        ServerEventReset(ctx);
        CR_RESET_WITH(ctx, MULTI_ERR_ONEVENT_SERVER_RECV_ERR);
    }

    if (ctx->rx->len < HANDLER_RET_SIZE) {
        printf("[%s] ERR: OnEventServer: rx too short for handler header (len=%lu, need=%u)\n",
               getTaskName(), (unsigned long)ctx->rx->len, (unsigned)HANDLER_RET_SIZE);
        ServerEventReset(ctx);
        CR_RESET_WITH(ctx, MULTI_ERR_ONEVENT_SERVER_RECV_ERR);
    }

    Func handler = *(Func *)ctx->rx->data;
    uint32_t payload_len = ctx->rx->len - HANDLER_RET_SIZE;
    Message input_view = {
        .len = payload_len,
        .data = payload_len > 0 ? ctx->rx->data + HANDLER_RET_SIZE : NULL,
    };

    ctx->tx = MessageCreate(CLIENT_MAX_CAP + HANDLER_RET_SIZE);
    Message output_view = {
        .len = CLIENT_MAX_CAP,
        .data = ctx->tx->data + HANDLER_RET_SIZE,
    };
    int ret = handler(&input_view, &output_view);

    MessageDelete(ctx->rx);
    ctx->rx = NULL;

    if (ret == 0) {
        ctx->tx->len = output_view.len + HANDLER_RET_SIZE;
    } else {
        ctx->tx->len = HANDLER_RET_SIZE;
    }
    *(int *)ctx->tx->data = ret;

    s->send_ctx.tx = ctx->tx;
    CR_YIELD(ctx);

    if (ev->type != EvSendStart) {
        printf("[%s] ERR: OnEventServer: expected EvSendStart, got type=%d\n",
               getTaskName(), (int)ev->type);
        ServerEventReset(ctx);
        CR_RESET_WITH(ctx, MULTI_ERR_ONEVENT_SERVER_SEND_ERR);
    }
    CR_YIELD(ctx);

    if (ev->type != EvSendDone) {
        printf("[%s] ERR: OnEventServer: expected EvSendDone, got type=%d\n",
               getTaskName(), (int)ev->type);
        ServerEventReset(ctx);
        CR_RESET_WITH(ctx, MULTI_ERR_ONEVENT_SERVER_SEND_DONE_ERR);
    }

    ServerEventReset(ctx);
    CR_RESET(ctx);
    CR_END(ctx);
    return 0;
}


int OnEventClient(ClientEventCtx *ctx, MultiSession *s, Event *ev) {
    // Client does not subscribe to EvPreSendStart
    if (ev->type == EvPreSendStart) {
        return 0;
    }

    if (ev->type == EvPreOnFirstStep) {
        assert(ctx->tx_init != NULL);
        assert(ctx->tx != NULL);
        assert(ctx->handler != NULL);
        // 组合 handler + payload 到 tx_init
        *(Func *)ctx->tx_init->data = ctx->handler;
        memcpy(ctx->tx_init->data + HANDLER_RET_SIZE, ctx->tx->data, ctx->tx->len);
        ctx->tx_init->len = HANDLER_RET_SIZE + ctx->tx->len;
        s->send_ctx.tx = ctx->tx_init;
        return 0;
    }

    CR_START(ctx);
    ctx->rx_done = NULL;
    ctx->handler_ret = 0;
    assert(ctx->rx_init != NULL);

    if (ev->type != EvSendStart) {
        printf("[%s] ERR: OnEventClient: expected EvSendStart, got type=%d\n",
               getTaskName(), (int)ev->type);
        CR_RESET_WITH(ctx, MULTI_ERR_ONEVENT_CLIENT_SEND_ERR);
    }
    CR_YIELD(ctx);

    if (ev->type != EvSendDone) {
        printf("[%s] ERR: OnEventClient: expected EvSendDone, got type=%d\n",
               getTaskName(), (int)ev->type);
        CR_RESET_WITH(ctx, MULTI_ERR_ONEVENT_CLIENT_SEND_DONE_ERR);
    }
    CR_YIELD(ctx);

    if (ev->type != EvRecvStart) {
        printf("[%s] ERR: OnEventClient: expected EvRecvStart, got type=%d\n",
               getTaskName(), (int)ev->type);
        CR_RESET_WITH(ctx, MULTI_ERR_ONEVENT_CLIENT_RECV_ERR);
    }
    if (ev->as.recv_start.total > ctx->rx_init->len) {
        printf("[%s] ERR: OnEventClient: recv total exceeds rx_init (total=%lu, rx_init=%lu)\n",
               getTaskName(), (unsigned long)ev->as.recv_start.total, (unsigned long)ctx->rx_init->len);
        CR_RESET_WITH(ctx, MULTI_ERR_ONEVENT_CLIENT_RECV_OVERSIZE);
    }
    ctx->rx_init->len = ev->as.recv_start.total;
    s->recv_ctx.rx = ctx->rx_init;
    CR_YIELD(ctx);

    if (ev->type != EvRecvDone) {
        printf("[%s] ERR: OnEventClient: expected EvRecvDone, got type=%d\n",
               getTaskName(), (int)ev->type);
        CR_RESET_WITH(ctx, MULTI_ERR_ONEVENT_CLIENT_RECV_ERR);
    }

    if (s->recv_ctx.rx == NULL) {
        printf("[%s] ERR: OnEventClient: recv_ctx.rx is NULL (rx_done missing)\n", getTaskName());
        CR_RESET_WITH(ctx, MULTI_ERR_ONEVENT_CLIENT_FMT_UNKNOWN);
    }
    if (s->recv_ctx.rx->len < HANDLER_RET_SIZE) {
        printf("[%s] ERR: OnEventClient: recv_ctx.rx->len too short (len=%lu, need=%u)\n",
               getTaskName(), (unsigned long)s->recv_ctx.rx->len, (unsigned)HANDLER_RET_SIZE);
        CR_RESET_WITH(ctx, MULTI_ERR_ONEVENT_CLIENT_FMT_TOOSHORT);
    }


    {
        ctx->handler_ret = *(int32_t *)s->recv_ctx.rx->data;
        uint32_t payload_len = s->recv_ctx.rx->len - HANDLER_RET_SIZE;
        ctx->rx_view = *s->recv_ctx.rx;
        ctx->rx_view.data = s->recv_ctx.rx->data + HANDLER_RET_SIZE;
        ctx->rx_view.len = payload_len;
    }

    ctx->rx_done = s->recv_ctx.rx;

    CR_RESET(ctx);

    CR_END(ctx);
    return 0;
}

// ================================================
// Adapter layer for task
// ================================================

// --- Public server entry ---
int multi_handler(Message *input, Message *output)
{
    static MultiSession srv_session = {0};
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

    // send side
    session.event_ctx.client.tx = input;
    session.event_ctx.client.handler = func;
    Message *temp_input = MessageCreate(input->len + HANDLER_RET_SIZE);
    session.event_ctx.client.tx_init = temp_input;

    // recv side
    Message *temp_output = MessageCreate(output->len + HANDLER_RET_SIZE);
    session.event_ctx.client.rx_init = temp_output;

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

    int ret = 0;

    int session_ret = 0;
    int step_ret = 0;
    int rpc_ret = 0;

    do {
        printf("[%s] ===== ROUND %lu =====\n", getTaskName(), (unsigned long)session.round);

        swap_omessage.len = sizeof(swap_obuffer);
        swap_omessage.data = swap_obuffer;
        step_ret = StepMultiSession(&session, &swap_imessage, &swap_omessage);
        if (step_ret != 0) { break; }

        session_ret = session.recv_ctx.error ? session.recv_ctx.error
            : session.send_ctx.error ? session.send_ctx.error
            : session.event_ctx.client.error ? session.event_ctx.client.error
            : 0;
        if (session_ret) { break; }

        swap_imessage.len = sizeof(swap_ibuffer);
        swap_imessage.data = swap_ibuffer;
        rpc_ret = rpc_call(multi_handler, &swap_omessage, &swap_imessage);
        if (rpc_ret != 0) { break; }

    } while(CR_IS_STARTED(&session.send_ctx) || CR_IS_STARTED(&session.recv_ctx));

    if (session_ret || step_ret || rpc_ret) {
        printf("[rpc_call_multi] error: session_ret=%d step_ret=%d rpc_ret=%d\n",
               session_ret, step_ret, rpc_ret);

        ret = session_ret ? session_ret
            : step_ret ? step_ret
            : rpc_ret ?  rpc_ret
            : 0;
        goto EXIT;
    }

    if (session.event_ctx.client.rx_done == NULL) {
        printf("[rpc_call_multi] error: client.rx_done is NULL (incomplete exchange)\n");
        ret = MULTI_ERR_ONEVENT_CLIENT_FMT_UNKNOWN;
        goto EXIT;
    }

    ret = session.event_ctx.client.handler_ret;
    output->len = MessageCopy(
            output,
            0,
            &session.event_ctx.client.rx_view,
            0,
            session.event_ctx.client.rx_view.len
            );

EXIT:
    MessageDelete(temp_output);
    MessageDelete(temp_input);
    return ret;
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
    printf("test_echo: input %" PRIu32 " -> output %" PRIu32 ", sedd %" PRIu32 "\n", input_len, output_len, seed);
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
               ret, MULTI_ERR_COSEND_START_ERR);

        /* output buffer too small -> client rejects as resp oversize */
        ret = test_echo(MULTI_CAP, MULTI_CAP / 2, counter);
        printf("[TaskClient] test in=MULTI_CAP out=MULTI_CAP/2: ret=%d (expect %d)\n\n",
               ret, MULTI_ERR_CORECV_NOMEM);


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
