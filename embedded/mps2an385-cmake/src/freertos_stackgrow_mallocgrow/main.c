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

void TaskMonitor() {
    TaskHandle_t xTask = xTaskGetHandle("TaskOverFlow"); // 获取任务句柄
    while(1) {
        UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(xTask);
        printf("Overflow HighWaterMark is %d !\n", highWaterMark);
        vTaskDelay(30);
    }
}

void StackGrowTest(int count) {
    if ( count == 0 ) {
        return;
    }

    vTaskDelay(60);
    unsigned int a = 0;
    printf("Stack Pointer address is 0x%08x\n", (unsigned int)&a);
    StackGrowTest(count - 1);

    return;
}

void TaskGrowTest() {
    void *p = 0;
    int i = 0;
    // Malloc Test
    for (i = 0; i < 4 ; i++ ) {
        vTaskDelay(60);
        p = malloc(4);
        printf("Malloc Pointer address is 0x%08x\n", (unsigned int)p);
    }

    // Stack Test
    StackGrowTest(4);


    while(1);
}

int main() {
    setup();
    printf("Init complete\n");
    // xTaskCreate( TaskMonitor,
    //         "TaskMonitor",
    //         configMINIMAL_STACK_SIZE + 3*1024,
    //         NULL,
    //         (tskIDLE_PRIORITY + 1),
    //         NULL );
    xTaskCreate( TaskGrowTest, 
            "TaskGrowTest",
            configMINIMAL_STACK_SIZE + 512,
            NULL,
            (tskIDLE_PRIORITY + 1),
            NULL );
    vTaskStartScheduler();
    while(1);
}
