#include <stdio.h>
#include <string.h>
#include "CMSDK_CM3.h"
#include "FreeRTOS.h"
#include "task.h"

void NVIC_Init() {
    size_t i = 0;
    for (i = 0; i < 32; i++) {
        NVIC_SetPriority((IRQn_Type)i, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    }
}

void setup() {
    extern int stdout_init(void);
    stdout_init();
    NVIC_Init();
}

static char* __RunTimeStatsPrintName(char* writeBuffer, const char* taskName) {
    unsigned int j;

    writeBuffer += sprintf(writeBuffer, "%s", taskName);
    for (j = strlen(taskName); j < configMAX_TASK_NAME_LEN - 1; ++j) {
        writeBuffer += sprintf(writeBuffer, " ");
    }

    return writeBuffer;
}

static char* __RunTimeStatsPrintCPULoad(char* writeBuffer, unsigned int cpuLoad) {
    if (cpuLoad < 1) {
        writeBuffer += sprintf(writeBuffer, " <1");
    } else if (cpuLoad < 10) {
        writeBuffer += sprintf(writeBuffer, "  %d", cpuLoad);
    } else {
        writeBuffer += sprintf(writeBuffer, " %d", cpuLoad);
    }
    writeBuffer += sprintf(writeBuffer, "%%");
    return writeBuffer;
}

static char* __RunTimeStatsPrintNumber(char* writeBuffer, unsigned int maxDigits, unsigned int number) {
    unsigned int i;

    unsigned int digits = 1;
    unsigned int maxVal = 9;

    while (number > maxVal) {
        digits++;
        maxVal = maxVal * 10 + 9;
    }

    if (digits > maxDigits) {
        for (i = 0; i < maxDigits; ++i) {
            writeBuffer += sprintf(writeBuffer, "?");
        }
    } else {
        for (i = maxDigits - digits; i > 0; --i) {
            writeBuffer += sprintf(writeBuffer, " ");
        }
        writeBuffer += sprintf(writeBuffer, "%d", number);
    }

    return writeBuffer;
}

static char* __RunTimeStatsPrintState(char* writeBuffer, eTaskState state) {
    switch (state) {
    case eRunning:
        writeBuffer += sprintf(writeBuffer, " Run ");
        break;
    case eReady:
        writeBuffer += sprintf(writeBuffer, "Ready");
        break;
    case eBlocked:
        writeBuffer += sprintf(writeBuffer, "Block");
        break;
    case eSuspended:
        writeBuffer += sprintf(writeBuffer, "Susp.");
        break;
    case eDeleted:
        writeBuffer += sprintf(writeBuffer, " Del ");
        break;
    case eInvalid:
        writeBuffer += sprintf(writeBuffer, " Inv ");
        break;
    }

    return writeBuffer;
}

static char* __RunTimeStatsPrintHeader(char* writeBuffer) {
    writeBuffer = __RunTimeStatsPrintName(writeBuffer, "Name");
    writeBuffer += sprintf(writeBuffer, " ");
    writeBuffer += sprintf(writeBuffer, "Load");
    writeBuffer += sprintf(writeBuffer, "  ");
    writeBuffer += sprintf(writeBuffer, "Stack");
    writeBuffer += sprintf(writeBuffer, " ");
    writeBuffer += sprintf(writeBuffer, "State");
    writeBuffer += sprintf(writeBuffer, "\r\n");
    return writeBuffer;
}

static char* __RunTimeStatsPrintLine(char* writeBuffer, const char* taskName, unsigned int cpuLoad, unsigned int freeStack, eTaskState state) {
    writeBuffer = __RunTimeStatsPrintName(writeBuffer, taskName);
    writeBuffer += sprintf(writeBuffer, " ");
    writeBuffer = __RunTimeStatsPrintCPULoad(writeBuffer, cpuLoad);
    writeBuffer += sprintf(writeBuffer, " ");
    writeBuffer = __RunTimeStatsPrintNumber(writeBuffer, 5, freeStack);
    writeBuffer += sprintf(writeBuffer, " ");
    writeBuffer = __RunTimeStatsPrintState(writeBuffer, state);
    writeBuffer += sprintf(writeBuffer, "\r\n");
    return writeBuffer;
}

static void __RunTimeStatsBubbleSort(TaskStatus_t* tasks, unsigned int size, int reverse) {
    unsigned int i;
    unsigned int biggestElementValue, biggestElementIndex;
    TaskStatus_t swapBuffer;

    if (size <= 1) {
        return;
    }

    biggestElementValue = tasks[0].ulRunTimeCounter;
    biggestElementIndex = 0;

    for (i = 1; i < size; ++i) {
        if (tasks[i].ulRunTimeCounter > biggestElementValue) {
            biggestElementValue = tasks[i].ulRunTimeCounter;
            biggestElementIndex = i;
        }
    }

    if (!reverse) {
        swapBuffer = tasks[size - 1];
        tasks[size - 1] = tasks[biggestElementIndex];
        tasks[biggestElementIndex] = swapBuffer;
        __RunTimeStatsBubbleSort(tasks, size - 1, reverse);
    } else {
        swapBuffer = tasks[0];
        tasks[0] = tasks[biggestElementIndex];
        tasks[biggestElementIndex] = swapBuffer;
        __RunTimeStatsBubbleSort(tasks + 1, size - 1, reverse);
    }
}

void TaskGetRunTimeStats(char *pcWriteBuffer) {
    unsigned int i;
    TaskStatus_t* t;

    unsigned int taskCount;
    TaskStatus_t* taskStatus;
    unsigned long totalRuntime;

    if (pcWriteBuffer == NULL) {
        return;
    }

    taskCount = uxTaskGetNumberOfTasks();

    taskStatus = pvPortMalloc(taskCount * sizeof(TaskStatus_t));
    if (taskStatus == NULL) {
        *pcWriteBuffer = '\0';
        return;
    }

    taskCount = uxTaskGetSystemState(taskStatus, taskCount, &totalRuntime);
    if (taskCount == 0) {
        *pcWriteBuffer = '\0';
        vPortFree(taskStatus);
        return;
    }

    __RunTimeStatsBubbleSort(taskStatus, taskCount, 1);

    totalRuntime /= 100;
    if (totalRuntime == 0) {
        *pcWriteBuffer = '\0';
        vPortFree(taskStatus);
        return;
    }

    pcWriteBuffer = __RunTimeStatsPrintHeader(pcWriteBuffer);
    for (i = 0, t = taskStatus; i < taskCount; ++i, ++t) {
        pcWriteBuffer = __RunTimeStatsPrintLine(
                pcWriteBuffer,
                t->pcTaskName,
                t->ulRunTimeCounter / totalRuntime,
                t->usStackHighWaterMark,
                t->eCurrentState
                );
    }

    *pcWriteBuffer = '\0';
    vPortFree(taskStatus);
}

void TaskPing() {
    while (1) {
        printf("Ping!\n");
        vTaskDelay(300);
    }
}

void TaskPong() {
    while (1) {
        printf("Pong!\n");
        vTaskDelay(40);
    }
}

void TaskRunTimeStats() {
    char statsBuffer[2048];

    while (1) {
        vTaskDelay(5000);

        memset(statsBuffer, 0, sizeof(statsBuffer));
        TaskGetRunTimeStats(statsBuffer);
        printf("--- Runtime Stats ---\r\n%s\r\n", statsBuffer);
    }
}

int main() {
    setup();
    printf("Init complete\n");

    xTaskCreate(TaskPing,
            "TaskPing",
            configMINIMAL_STACK_SIZE + 3 * 1024,
            NULL,
            (tskIDLE_PRIORITY + 1),
            NULL);
    xTaskCreate(TaskPong,
            "TaskPong",
            configMINIMAL_STACK_SIZE + 3 * 1024,
            NULL,
            (tskIDLE_PRIORITY + 1),
            NULL);
    xTaskCreate(TaskRunTimeStats,
            "TaskStats",
            configMINIMAL_STACK_SIZE + 4 * 1024,
            NULL,
            (tskIDLE_PRIORITY + 2),
            NULL);

    vTaskStartScheduler();
    while (1);
}
