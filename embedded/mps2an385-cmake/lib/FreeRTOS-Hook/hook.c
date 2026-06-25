
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

// For freertos_runtimestat
// void __RunTimeStatsTimerInit(void) {
//     CMSDK_TIMER0->CTRL = 0;
//     CMSDK_TIMER0->RELOAD = 0xFFFFFFFFUL;
//     CMSDK_TIMER0->VALUE = 0xFFFFFFFFUL;
//     CMSDK_TIMER0->CTRL = CMSDK_TIMER_CTRL_EN_Msk;
// }

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

void vApplicationStackOverflowHook( TaskHandle_t pxTask,
                                    char * pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
     * configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
     * function is called if a stack overflow is detected. */
    printf( "\n\n===Stack overflow in %s===\n", pcTaskName );
    // DEVTODO: portDISABLE_INTERRUPTS();

    for( ; ; )
    {
    }
}
