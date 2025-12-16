#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"

// Coroutine handle
CRCB_t xPingCoRoutineHandle;
CRCB_t xPongCoRoutineHandle;

#define CO_STACK_SIZE configMINIMAL_STACK_SIZE

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

// Coroutine functions use vCoRoutineFunction_t signature
static void prvPingCoRoutine(CoRoutineHandle_t xHandle, UBaseType_t uxIndex) {
    // Coroutines must start with crSTART
    crSTART(xHandle);

    // Coroutine body - runs from its saved context
    for (;;) {
        printf("Ping!\n");
        // Coroutines use crDELAY instead of vTaskDelay
        crDELAY(xHandle, 300); // Delay for 300 ticks

        // You could yield to other coroutines at any point
        // crYIELD();
    }

    // Coroutines must end with crEND
    crEND();
}

static void prvPongCoRoutine(CoRoutineHandle_t xHandle, UBaseType_t uxIndex) {
    crSTART(xHandle);

    for (;;) {
        printf("Pong!\n");
        crDELAY(xHandle, 40); // Delay for 40 ticks
    }

    crEND();
}

// Scheduler task to schedule coroutines
static void prvCoRoutineSchedulerTask(void *pvParameters) {
    (void)pvParameters;

    for (;;) {
        // Schedule coroutines - they will run when their delays expire
        vCoRoutineSchedule();

        // Give some time to the idle task
        vTaskDelay(1);
    }
}

int main() {
    setup();
    printf("FreeRTOS Coroutine Demo Init complete\n");

    // Create the coroutines
    xCoRoutineCreate(prvPingCoRoutine,
                     (UBaseType_t)0,  // Priority (not used in this parameter)
                     (UBaseType_t)0); // Index parameter passed to coroutine

    xCoRoutineCreate(prvPongCoRoutine,
                     (UBaseType_t)0,
                     (UBaseType_t)1); // Different index if needed

    xTaskCreate(prvCoRoutineSchedulerTask,
                "CoRoutineSched",
                configMINIMAL_STACK_SIZE + 1*1024,
                NULL,
                (tskIDLE_PRIORITY + 1),
                NULL);

    vTaskStartScheduler();
    while(1);
}
