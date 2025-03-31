#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "task.h"

void vAssertCalled( const char * pcFileName,
                    uint32_t ulLine )
{
    volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0;

    /* Called if an assertion passed to configASSERT() fails.  See
     * http://www.freertos.org/a00110.html#configASSERT for more information. */

    printf( "ASSERT! Line %d, file %s\r\n", ( int ) ulLine, pcFileName );

    taskENTER_CRITICAL();
    {
        /* You can step out of this function to debug the assertion by using
         * the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
         * value. */
        while( ulSetToNonZeroInDebuggerToContinue == 0 )
        {
            __asm volatile ( "NOP" );
            __asm volatile ( "NOP" );
        }
    }
    taskEXIT_CRITICAL();
}

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

void TaskPing() {
    while(1) {
        printf("Ping!\n");
        vTaskDelay(1000);
    }

}

void TaskPong() {
    while(1) {
        printf("Pong!\n");
        vTaskDelay(1700);
    }

}


int main() {
    setup();
    printf("Init complete\n");
    xTaskCreate( TaskPing,
            "TaskPing",
            configMINIMAL_STACK_SIZE + 3*1024,
            NULL,
            (tskIDLE_PRIORITY + 1),
            NULL );
    xTaskCreate( TaskPong,
            "TaskPong",
            configMINIMAL_STACK_SIZE + 3*1024,
            NULL,
            (tskIDLE_PRIORITY + 1),
            NULL );
    vTaskStartScheduler();
    while(1);
}
