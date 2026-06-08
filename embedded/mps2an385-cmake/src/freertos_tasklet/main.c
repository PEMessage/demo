#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include "CMSDK_CM3.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* ============================================================
 *  Board setup
 * ============================================================ */

static void NVIC_Init(void)
{
    for (size_t i = 0; i < 32; i++) {
        NVIC_SetPriority((IRQn_Type)i, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    }
}

static void setup(void)
{
    extern int stdout_init(void);
    stdout_init();
    NVIC_Init();
}

/* ============================================================
 *  call  –  run a function in a temporary large-stack
 *                    FreeRTOS task and wait for its result.
 *
 *  Returns: func’s return value, or CALL_ERROR on internal failure.
 *
 *  Resource safety:
 *    - Queue is created before the task and deleted before return.
 *    - The temporary task deletes itself with vTaskDelete(NULL).
 *    - TCB / stack are reclaimed by the Idle task (heap_4).
 * ============================================================ */

#define CALL_ERROR  INT_MIN

typedef struct {
    int (*func)(void *);
    void *arg;
    QueueHandle_t result_queue;
} TaskletParam_t;

static void TaskletWrapper(void *param)
{
    TaskletParam_t *ctx = (TaskletParam_t *)param;
    int result = ctx->func(ctx->arg);
    xQueueSend(ctx->result_queue, &result, portMAX_DELAY);
    vTaskDelete(NULL);
    while (1) { }          /* never reached */
}

int call(size_t stack_words, int (*func)(void *), void *arg)
{
    if (!func || stack_words == 0) {
        return CALL_ERROR;
    }

    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        return CALL_ERROR;
    }

    QueueHandle_t queue = xQueueCreate(1, sizeof(int));
    if (!queue) {
        return CALL_ERROR;
    }

    TaskletParam_t ctx = {
        .func         = func,
        .arg          = arg,
        .result_queue = queue,
    };

    TaskHandle_t htask = NULL;
    UBaseType_t caller_prio = uxTaskPriorityGet(NULL);

    if (xTaskCreate(TaskletWrapper,
                    "tasklet",
                    stack_words,
                    &ctx,
                    caller_prio,
                    &htask) != pdPASS)
    {
        vQueueDelete(queue);
        return CALL_ERROR;
    }

    int result;
    if (xQueueReceive(queue, &result, portMAX_DELAY) != pdTRUE) {
        /* Should never happen with portMAX_DELAY, but keep the path clean. */
        vQueueDelete(queue);
        return CALL_ERROR;
    }

    vQueueDelete(queue);
    return result;
}

/* ============================================================
 *  Demo
 * ============================================================ */

/* A stack-heavy function: each recursion frame burns ~4 KiB. */
static int heavy_recursive(void *arg)
{
    int depth = (int)(intptr_t)arg;

    /* Force the compiler to allocate 4 KiB on stack. */
    volatile char buf[4096];
    memset((void *)buf, 0, sizeof(buf));

    printf("heavy_recursive: depth=%d, buf addr=%p, size=%u\n",
           depth, (void *)buf, (unsigned)sizeof(buf));

    if (depth > 0) {
        return heavy_recursive((void *)(intptr_t)(depth - 1));
    }
    return 42;
}

/* A small-stack task that offloads the heavy work via call(). */
static void SmallStackTask(void *param)
{
    (void)param;

    printf("SmallStackTask: starting, stack size = %u words\n",
           (unsigned)(configMINIMAL_STACK_SIZE + 512));

    int res = call(configMINIMAL_STACK_SIZE + 10 * 1024,
                   heavy_recursive,
                   (void *)(intptr_t)3);

    printf("SmallStackTask: call returned %d\n", res);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ============================================================
 *  main
 * ============================================================ */

int main(void)
{
    setup();
    printf("Init complete\n");

    xTaskCreate(SmallStackTask,
                "SmallStack",
                configMINIMAL_STACK_SIZE + 512,
                NULL,
                10,
                NULL);

    vTaskStartScheduler();
    while (1);
    return 0;
}
