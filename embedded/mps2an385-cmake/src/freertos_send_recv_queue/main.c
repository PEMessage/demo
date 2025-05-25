#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock

#include "FreeRTOS.h"
#include "queue.h" // for QueueHandle_t
#include "task.h"
#include "event_groups.h"

/*-----------------------------------------------------------*/
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
/*-----------------------------------------------------------*/
/* Priorities at which the tasks are created. */
#define mainQUEUE_RECEIVE_TASK_PRIORITY    ( tskIDLE_PRIORITY + 2 )
#define mainQUEUE_SEND_TASK_PRIORITY       ( tskIDLE_PRIORITY + 1 )
#define mainUDP_CLI_TASK_PRIORITY          ( tskIDLE_PRIORITY )

/* The rate at which data is sent to the queue.  The (simulated) 250ms value is
 * converted to ticks using the portTICK_RATE_MS constant. */
#define mainQUEUE_SEND_FREQUENCY_MS        ( 250 / portTICK_PERIOD_MS )

/* The number of items the queue can hold.  This is 1 as the receive task
 * will remove items as they are added, meaning the send task should always find
 * the queue empty. */
#define mainQUEUE_LENGTH                   ( 2 )

/* The queue used by both tasks. */
static QueueHandle_t xQueue = NULL;
/*-----------------------------------------------------------*/

static void prvQueueSendTask( void * pvParameters )
{
    TickType_t xNextWakeTime;
    unsigned long ulValueToSend = 100UL;

    /* Remove warning about unused parameters. */
    ( void ) pvParameters;

    /* Initialise xNextWakeTime - this only needs to be done once. */
    xNextWakeTime = xTaskGetTickCount();

    for( ; ; )
    {
        /* Place this task in the blocked state until it is time to run again.
         *  While in the Blocked state this task will not consume any CPU time. */
        vTaskDelayUntil( &xNextWakeTime, mainQUEUE_SEND_FREQUENCY_MS );

        /* Send to the queue - causing the queue receive task to unblock and
         * write a message to the display.  0 is used as the block time so the
         * sending operation will not block - it shouldn't need to block as the
         * queue should always	be empty at this point in the code, and it is an
         * error if it is not. */
        xQueueSend( xQueue, &ulValueToSend, 0U );

    }
}

static void prvQueueReceiveTask( void * pvParameters )
{
    unsigned long ulReceivedValue;

    /* Remove warning about unused parameters. */
    ( void ) pvParameters;

    for( ; ; )
    {
        /* Wait until something arrives in the queue - this task will block
         * indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
         * FreeRTOSConfig.h. */
        xQueueReceive( xQueue, &ulReceivedValue, portMAX_DELAY );

        /*  To get here something must have been received from the queue, but
         * is it the expected value?  If it is, write the message to the
         * display before looping back to block on the queue again. */
        if( ulReceivedValue == 100UL )
        {
            printf("Message received!\r\n");
            ulReceivedValue = 0U;
        }
    }
}


int main() {
    setup();
    printf("Init complete\n");

    xQueue = xQueueCreate( mainQUEUE_LENGTH, sizeof( unsigned long ) );
    xTaskCreate( prvQueueReceiveTask,             /* The function that implements the task. */
                 "Rx",                            /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                 configMINIMAL_STACK_SIZE,        /* The size of the stack to allocate to the task.  Not actually used as a stack in the Win32 simulator port. */
                 NULL,                            /* The parameter passed to the task - not used in this example. */
                 mainQUEUE_RECEIVE_TASK_PRIORITY, /* The priority assigned to the task. */
                 NULL );                          /* The task handle is not required, so NULL is passed. */
    xTaskCreate( prvQueueSendTask, "TX", configMINIMAL_STACK_SIZE, NULL, mainQUEUE_SEND_TASK_PRIORITY, NULL );

    // Create the event group
    vTaskStartScheduler();
    while(1);
}





