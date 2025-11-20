#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "CMSDK_CM3_EXT.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

void NVIC_Init() {
    size_t i = 0 ;
    for ( i = 0; i < 32; i++) {
        // NVIC_DisableIRQ(i);
        NVIC_SetPriority((IRQn_Type)i, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    }
}

void TouchIRQ_Init() {
    TOUCH_CTRL->enable_irq = 1;
    TOUCH_CTRL->reserved = 77; // random number to verify struct work
    NVIC_EnableIRQ(Touch_IRQn);
}


void setup() {
    extern int stdout_init (void);
    stdout_init();
    NVIC_Init();
    TouchIRQ_Init();
}

// ========================================
// Input Framework
// ========================================

typedef struct Point {
    uint16_t x;
    uint16_t y;
} Point;

typedef struct ScanData {
    uint8_t pressed;
    Point point;
} ScanData;


typedef uint16_t ObjectID;

typedef struct InputDevice {
    bool enabled : 1;

    uint8_t pressed;
    Point point;

    /**
     * @brief 0 is Relaese, 1 is Background, each id relate to a zone
     *        and conform following pattern
     *        0 -> 1 -> 0 -> x -> 0 ...
     *        will change back 0 before change to others
     */
    ObjectID object_id;

} InputDevice;

struct InputDevice *DEV;


InputDevice *InputDeviceCreate() {
    InputDevice *indev = malloc(sizeof(*indev));

    indev->enabled = true;

    indev->point.x = 0;
    indev->point.y = 0;


    indev->object_id = 0;

    return indev;
}

// 1. Call From IRQ or Timer
// ----------------------------------------
void InputDeviceScan(InputDevice* indev) {
    if (!indev) { return; }
    if (!indev->enabled) { return; }


    // Actually Read something
    {
        struct ScanData data;
        {
            data.pressed = 0;
            data.point = indev->point;
        }
        // Forward declare
        void InputDeviceScanCore(InputDevice* indev, ScanData *data);
        InputDeviceScanCore(indev, &data);

        {
            indev->point = data.point;
            indev->pressed = data.pressed;
        }
    }

    printf("x:%4d, y:%4d, pressed:%2d\n",
            indev->point.x, indev->point.y, indev->pressed);

    if (indev->pressed) {
        void InputDeviceScanPressProc(InputDevice *indev);
        InputDeviceScanPressProc(indev);
    } else {
        void InputDeviceScanReleaseProc(InputDevice *indev);
        InputDeviceScanReleaseProc(indev);
    }

}

// 2. Call From InputDeviceScan
// ----------------------------------------

void InputDeviceScanPressProc(InputDevice *indev) {
    ObjectID cur_objec_id = indev->object_id;

    {
        if (cur_objec_id == 0) {
            cur_objec_id = 1; // TODO: maybe better way?
        }

        // On ObjectID Changed
        if (cur_objec_id != indev->object_id) {
            void onObjectIdChanged(InputDevice *indev, ObjectID cur_objec_id);
            onObjectIdChanged(indev, cur_objec_id);
        }
    }

    indev->object_id = cur_objec_id;
}

void InputDeviceScanReleaseProc(InputDevice *indev) {
    ObjectID cur_objec_id = indev->object_id;

    {
        if (cur_objec_id) {
            cur_objec_id = 0; // TODO: maybe better way?
        }

        // On ObjectID Changed
        if (cur_objec_id != indev->object_id) {
            void onObjectIdChanged(InputDevice *indev, ObjectID cur_objec_id);
            onObjectIdChanged(indev, cur_objec_id);
        }

    }

    indev->object_id = cur_objec_id;
}

// 3. Call From
//  InputDeviceScanPressProc
//  InputDeviceScanReleaseProc
// ----------------------------------------

void onObjectIdChanged(InputDevice *indev, ObjectID cur_objec_id) {
    printf("ID %d -> %d: x=%d y=%d\n",
            indev->object_id, cur_objec_id,
            indev->point.x, indev->point.y
          );
}


// ========================================
// Adapter
// ========================================
void InputDeviceScanCore(InputDevice* indev, ScanData *data) {
    data->point.x = *TOUCH_X;
    data->point.y = *TOUCH_Y;
    data->pressed = *TOUCH_PRESS;
    return ;
}

// Call From IRQ
// ----------------------------------------
void touch_pend_callback(void * arg1, uint32_t arg2) {
    struct InputDevice *indev = (InputDevice *)arg1;
    InputDeviceScan(indev);
}

#define NO_HOVER_SUPPORT
void TouchIRQ_Handler() {
    static uint32_t prev_pressed = 0;
    uint32_t prev = prev_pressed;
    uint32_t curr = *TOUCH_PRESS;
    prev_pressed = curr; // update prev

    #ifdef NO_HOVER_SUPPORT
    // if neither edge nor pressed skip it -- skip hover event
    if (!( (prev ^ curr) || curr )) {
        return;
    }
    #endif

    // See:
    // https://freertos.org/zh-cn-cmn-s/Documentation/02-Kernel/04-API-references/11-Software-timers/18-xTimerPendFunctionCallFromISRhttps://freertos.org/zh-cn-cmn-s/Documentation/02-Kernel/04-API-references/11-Software-timers/18-xTimerPendFunctionCallFromISR
    BaseType_t xHigherPriorityTaskWoken;
    xTimerPendFunctionCallFromISR(
            touch_pend_callback,
            DEV,
            NULL,
            &xHigherPriorityTaskWoken
            );
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


// Call From Timer
// ----------------------------------------
const uint32_t PERIOD = 1000; // ms
static TimerHandle_t touch_timer = NULL;
void touch_timer_callback(TimerHandle_t xTimer) {
    printf("Tick count: %u ", xTaskGetTickCount());
    InputDeviceScan(DEV);
}


// ========================================
// Main Task
// ========================================

void MainTask() {
    // create indev
    DEV = InputDeviceCreate();

    // Create a timer (initially not active)
    touch_timer = xTimerCreate(
        "TouchTimer",
        pdMS_TO_TICKS(PERIOD),  // 1 second period
        pdTRUE,               // Auto-reload
        NULL,                 // Timer ID
        touch_timer_callback
    );

    // Start the timer
    // xTimerStart(touch_timer, portMAX_DELAY);

    while(1) {
    }
}

int main() {
    setup();
    printf("Init complete\n");

    xTaskCreate( MainTask,
            "MainTask",
            configMINIMAL_STACK_SIZE + 3*1024,
            NULL,
            (tskIDLE_PRIORITY + 1),
            NULL );


    vTaskStartScheduler();
    while(1);
}
