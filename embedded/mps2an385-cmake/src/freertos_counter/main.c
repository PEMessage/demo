#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "task.h"

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


void performance_update(TickType_t* timer);
#if 0
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)       x
#define unlikely(x)     x
#endif


void TaskHighSpeedPoll() {
    uint32_t cnt = 0;
    TickType_t timer = 0;

    while(1) {
        if (unlikely(cnt % 100000 == 0)) {
            performance_update(&timer);
            cnt = 0;
        }
        // some payload here
        printf("cnt is %d\n", cnt); // without unlikely: 2480 ms / 100000 == 0.0248 ms 
                                    // with    unlikely: 3594 ms / 100000 == 0.0359 ms
                                    // with      likely: 2424 ms / 100000 == 0.0242 ms
                                    // Seem like unlikely not great for embedded?, what's likely or unlikely?
        // some payload here
        cnt ++;
    }
}


int main() {
    setup();
    printf("Init complete\n");
    xTaskCreate( TaskHighSpeedPoll,
            "TaskHighSpeedPoll",
            configMINIMAL_STACK_SIZE + 3*1024,
            NULL,
            (tskIDLE_PRIORITY + 1),
            NULL );
    vTaskStartScheduler();
    while(1);
}


void performance_update(TickType_t* timer) {
    TickType_t current_time = xTaskGetTickCount();
    TickType_t previous_time = *timer;
    TickType_t time_diff = current_time - previous_time;
    printf("Current time: %u ms, Previous time: %u ms, Difference: %u ms\n",
            current_time * portTICK_PERIOD_MS,
            previous_time * portTICK_PERIOD_MS,
            time_diff * portTICK_PERIOD_MS);

    *timer = xTaskGetTickCount();  // Update the timer
}

