// Source - https://stackoverflow.com/a/77568267
// Posted by Gabriel Staples, modified by community. See post 'Timeline' for change history
// Retrieved 2026-06-11, License - CC BY-SA 4.0

#ifndef FREERTOS_TASKS_C_ADDITIONS_INCLUDE_H
#define FREERTOS_TASKS_C_ADDITIONS_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"

#if ((configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H == 1) && (configRECORD_STACK_HIGH_ADDRESS == 1))
uint32_t taskGetStackSizeWords(TaskHandle_t taskHandle);
uint32_t taskGetStackSizeBytes(TaskHandle_t taskHandle);
#endif // ((configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H == 1)
       //   && (configRECORD_STACK_HIGH_ADDRESS == 1))

#ifdef __cplusplus
}
#endif

#endif /* end of include guard: FREERTOS_TASKS_C_ADDITIONS_INCLUDE_H */
