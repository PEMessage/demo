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
// Helper Function
// ========================================
#define STR(x) #x
#define STRINGIFY(x) STR(x)
#define FILE_LINE __FILE__ ":" STRINGIFY(__LINE__)

#define LV_ABS(x) ((x) > 0 ? (x) : (-(x)))

// ========================================
// Input Framework
// ========================================

typedef struct Point {
    int16_t x;
    int16_t y;
} Point;

typedef struct ScanData {
    uint8_t pressed;
    Point point;
} ScanData;


typedef uint16_t ObjectID;


typedef enum {
    DIR_NONE     = 0x00,
    DIR_LEFT     = (1 << 0),
    DIR_RIGHT    = (1 << 1),
    DIR_TOP      = (1 << 2),
    DIR_BOTTOM   = (1 << 3),
    DIR_HOR      = DIR_LEFT | DIR_RIGHT,
    DIR_VER      = DIR_TOP | DIR_BOTTOM,
    DIR_ALL      = DIR_HOR | DIR_VER,
} Direction;

typedef struct InputDevice {
    bool enabled : 1;

    uint8_t pressed;
    Point point;

    /**
     * @brief Will be set under following situation
     *        1. Every scan end, Will set last_point to point
     *        2. onObjectIdChanged, will set to point(force resync)
     */
    Point last_point;

    /**
     * @brief 0 is Relaese, 1 is Background, each id relate to a zone
     *        and conform following pattern
     *        0 -> 1 -> 0 -> x -> 0 ...
     *        will change back 0 before change to others
     */
    ObjectID object_id;
    /* check object_id is not zero before using following field */
    /**/ Point gesture_sum;
    /**/ uint8_t gesture_dir : 4;
    /**/ uint8_t gesture_sent : 1;



} InputDevice;

struct InputDevice *DEV;


InputDevice *InputDeviceCreate() {
    InputDevice *indev = malloc(sizeof(*indev));

    indev->enabled = true;

    indev->point.x = 0;
    indev->point.y = 0;
    indev->last_point.x = 0;
    indev->last_point.y = 0;


    indev->object_id = 0;
    {
        indev->gesture_sum.x = 0;
        indev->gesture_sum.y = 0;
        indev->gesture_sent = 0;
        indev->gesture_dir = DIR_NONE;
    }

    return indev;
}

// 1. Call From IRQ or Timer
// ----------------------------------------
void InputDeviceScan(InputDevice* indev) {
    if (!indev) { return; }
    if (!indev->enabled) { return; }


    { // Actually Read something
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

    // printf("x:%4d, y:%4d, pressed:%2d\n",
    //         indev->point.x, indev->point.y, indev->pressed);

    if (indev->pressed) {
        void InputDeviceScanPressProc(InputDevice *indev);
        InputDeviceScanPressProc(indev);
    } else {
        void InputDeviceScanReleaseProc(InputDevice *indev);
        InputDeviceScanReleaseProc(indev);
    }

    {   // Update last_point
        indev->last_point = indev->point;
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

    // UnderPress
    if (indev->object_id) {
        void detectGesture(InputDevice *indev);
        detectGesture(indev);
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

void onObjectIdChanged(InputDevice *indev, ObjectID cur_object_id) {
    printf("ID %d -> %d: x=%d y=%d\n",
            indev->object_id, cur_object_id,
            indev->point.x, indev->point.y
          );

    indev->last_point = indev->point; // resync last_point and point

    if (!indev->object_id && cur_object_id) { // OnPress
        printf("OnPress\n");
        {
            indev->gesture_sum.x = 0;
            indev->gesture_sum.y = 0;
            indev->gesture_dir = DIR_NONE;
            indev->gesture_sent = 0;
        }

    } else if (indev->object_id && !cur_object_id) { // OnRelease
        printf("OnRelease\n");

    } else { // UnKnow
        printf("ERROR: " FILE_LINE);
        while(1);
    }
}

void detectGesture(InputDevice *indev) {
    if(!indev->object_id) { return; }
    if(indev->gesture_sent) { return; }

    // printf("detectGesture!\n");
    Point diff = {
        indev->point.x - indev->last_point.x,
        indev->point.y - indev->last_point.y,
    };

    // move too small, reset gesture_sum.
    #define MIN_VELOCITY 3
    if ( LV_ABS(diff.x) < MIN_VELOCITY && LV_ABS(diff.y) < MIN_VELOCITY ) {
        indev->gesture_sum.x = 0;
        indev->gesture_sum.y = 0;
        return;
    }
    // otherwise, accumlate it
    indev->gesture_sum.x += diff.x;
    indev->gesture_sum.y += diff.y;

    // printf("sum of x %d\n", indev->gesture_sum.x);
    // printf("sum of y %d\n", indev->gesture_sum.y);

    #define GESTURE_LIMIT 50
    if (LV_ABS(indev->gesture_sum.x) > GESTURE_LIMIT ||
        LV_ABS(indev->gesture_sum.y) > GESTURE_LIMIT)
    {

        indev->gesture_sent = 1;
        if(LV_ABS(indev->gesture_sum.x) > LV_ABS(indev->gesture_sum.y)) {
            if(indev->gesture_sum.x > 0) {
                indev->gesture_dir = DIR_RIGHT;
            } else {
                indev->gesture_dir = DIR_LEFT;
            }
        } else {
            if(indev->gesture_sum.y > 0) {
                indev->gesture_dir = DIR_BOTTOM;
            } else {
                indev->gesture_dir = DIR_TOP;
            }
        }
        printf("Direction %d\n", indev->gesture_dir);

    }
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
