#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"


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

/* Define event bits */
#define TASK1_BIT (1 << 0)  // Bit 0
#define TASK2_BIT (1 << 1)  // Bit 1
#define ALL_BITS (TASK1_BIT | TASK2_BIT)

/* Event group handle */
EventGroupHandle_t xEventGroup;

/* Task prototypes */
void vTaskSetEvent1(void *pvParameters);
void vTaskSetEvent2(void *pvParameters);
void vTaskWaitEvent12(void *pvParameters);


int main() {
    setup();
    printf("Init complete\n");

    printf("FreeRTOS Event Group Demo\n");

    // Create the event group
    xEventGroup = xEventGroupCreate();

    if(xEventGroup == NULL) {
        printf("Failed to create event group\n");
        return 0;
    }

    // Create tasks
    // - TaskFunction_t pxTaskCode
    // - const char *const pcName
    // - const StackType_t uxStackDepth
    // - void *const pvParameters
    // - UBaseType_t uxPriority
    // - TaskHandle_t *const pxCreatedTask
    xTaskCreate(vTaskSetEvent1, "TaskSetEvent1", configMINIMAL_STACK_SIZE + 4096, NULL, 2, NULL);
    xTaskCreate(vTaskSetEvent2, "TaskSetEvent2", configMINIMAL_STACK_SIZE + 4096, NULL, 2, NULL);
    xTaskCreate(vTaskWaitEvent12, "Control Task", configMINIMAL_STACK_SIZE + 4096, NULL, 3, NULL);

    printf("Tasks created, scheduler starting...\n");
    vTaskStartScheduler();
    while(1);
}


void vTaskSetEvent1(void *pvParameters)
{
    const TickType_t xDelay = pdMS_TO_TICKS(1000);

    while(1) {
        printf("Task 1 running\n");

        // Simulate some work
        vTaskDelay(xDelay);

        // Set bit 0 in the event group
        printf("Task 1 setting bit 0\n");
        xEventGroupSetBits(xEventGroup, TASK1_BIT);
    }
}

void vTaskSetEvent2(void *pvParameters)
{
    const TickType_t xDelay = pdMS_TO_TICKS(1500);

    while(1) {
        printf("Task 2 running\n");

        // Simulate some work
        vTaskDelay(xDelay);

        // Set bit 1 in the event group
        printf("Task 2 setting bit 1\n");
        xEventGroupSetBits(xEventGroup, TASK2_BIT);
    }
}

void vTaskWaitEvent12(void *pvParameters)
{
    EventBits_t xEventGroupValue;
    const TickType_t xTimeout = pdMS_TO_TICKS(5000);

    while(1) {
        printf("Control Task waiting for both bits to be set...\n");

        // Wait for both bits to be set, clear them on exit
        xEventGroupValue = xEventGroupWaitBits(
            xEventGroup,    // The event group
            ALL_BITS,       // Bits to wait for
            pdTRUE,         // Clear bits on exit
            pdTRUE,         // Wait for ALL bits
            xTimeout        // Wait a maximum of 5 seconds
        );

        if((xEventGroupValue & ALL_BITS) == ALL_BITS) {
            printf("Control Task: Both bits set! Performing action...\n");
        } else {
            printf("Control Task: Timeout waiting for both bits\n");
        }
    }
}
