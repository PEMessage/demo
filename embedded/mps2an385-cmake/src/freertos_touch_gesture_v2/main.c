#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "CMSDK_CM3_EXT.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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

#define FOREACH_SETBITS(mask, _i) \
    for (unsigned int _i = 0, _temp_mask = (mask); \
         _temp_mask != 0; \
         _temp_mask >>= 1, _i++) \
        if (_temp_mask & 1)

#define ARRAY_INDEX(ptr, array) ((size_t)((ptr) - (array)) / sizeof(*(array)))
// ========================================
// Input Framework Helper
// ========================================
typedef uint16_t ObjectID;


// ========================================
// Input Framework
// ========================================
typedef struct Point {
    int16_t x;
    int16_t y;
} Point;

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

typedef struct Gesture {
    Direction direction;
    Point sum;
} Gesture;

typedef struct LongPress {
    uint8_t isLong;
    TickType_t startTick;
} LongPress;

typedef struct TouchPoint {
    int32_t track_id;
    Point point;
} TouchPoint;

typedef struct Finger {
    TouchPoint touch_point;


    Point start_point;
    Point prev_point;

    /* event base private data */
    Gesture gesture;
    LongPress longPress

} Finger;


#define MAX_SUPPORT_SLOT 3

typedef struct ScanData {
    uint32_t slot_mask;
    TouchPoint touch_points[MAX_SUPPORT_SLOT];
} ScanData;


typedef struct InputDevice {
    bool enabled : 1;

    uint32_t prev_slot_mask;
    uint32_t curr_slot_mask;

    Finger fingers[MAX_SUPPORT_SLOT];


} InputDevice;

struct InputDevice *DEV;


InputDevice *InputDeviceCreate() {
    InputDevice *indev = malloc(sizeof(*indev));
    memset(indev, 0, sizeof(*indev));

    indev->enabled = true;

    return indev;
}


// 1. Call From IRQ or Timer
// ----------------------------------------
void InputDeviceScan(InputDevice* indev) {
    if (!indev) { return; }
    if (!indev->enabled) { return; }

    indev->prev_slot_mask = indev->curr_slot_mask;
    { // Actually Read something
        struct ScanData data;
        {
            data.slot_mask = indev->curr_slot_mask;
            for (int i = 0 ; i < MAX_SUPPORT_SLOT ; i ++ ) {
                data.touch_points[i] = indev->fingers[i].touch_point;
            }
        }

        void InputDeviceScanCore(InputDevice* indev, ScanData *data);
        InputDeviceScanCore(indev, &data);

        {
            indev->curr_slot_mask = data.slot_mask;
            for (int i = 0 ; i < MAX_SUPPORT_SLOT ; i ++ ) {
                indev->fingers[i].touch_point = data.touch_points[i];
            }
        }
    }

    void detectProcess(InputDevice* indev);
    detectProcess(indev);
}

// 2. Call from InputDeviceScan
// ----------------------------------------

void detectProcess(InputDevice* indev) {
    uint32_t slot_mask_both_edge = indev->prev_slot_mask ^ indev->curr_slot_mask;
    uint32_t slot_mask_up_edge = slot_mask_both_edge & indev->curr_slot_mask;
    uint32_t slot_mask_down_edge = slot_mask_both_edge & indev->prev_slot_mask;
    uint32_t slot_mask_without_change_high = ~slot_mask_both_edge & indev->curr_slot_mask;
    uint32_t slot_mask_without_change_low = ~slot_mask_both_edge & ~indev->curr_slot_mask;

    FOREACH_SETBITS(slot_mask_up_edge, i) {
        void fingerStart(InputDevice *indev, Finger *finger);
        fingerStart(indev, &indev->fingers[i]);
    }

    FOREACH_SETBITS(slot_mask_down_edge, i) {
        void fingerEnd(InputDevice *indev, Finger *finger);
        fingerEnd(indev, &indev->fingers[i]);
    }

    FOREACH_SETBITS(slot_mask_without_change_high, i) {
        void fingerActive(InputDevice *indev, Finger *finger);
        fingerActive(indev, &indev->fingers[i]);
    }

    FOREACH_SETBITS(slot_mask_without_change_low, i) {
        void fingerIdle(InputDevice *indev, Finger *finger);
        fingerIdle(indev, &indev->fingers[i]);
    }

}


// 3.1 Call from detectProcess, Finger relate
// ----------------------------------------
void fingerStart(InputDevice *indev, Finger *finger) {
    finger->start_point = finger->touch_point.point;
    finger->prev_point = finger->touch_point.point;

    void GestureStart(Finger *finger, Gesture* gesture);
    GestureStart(finger, &finger->gesture);

    void LongPressStart(Finger *finger, LongPress *longPress);
    LongPressStart(finger, &finger->longPress);
}

void fingerActive(InputDevice *indev, Finger *finger) {
    void GestureActive(Finger *finger, Gesture* gesture);
    GestureActive(finger, &finger->gesture);

    void LongPressActive(Finger *finger, LongPress *longPress);
    LongPressActive(finger, &finger->longPress);


    // Keep this at the end
    finger->prev_point = finger->touch_point.point;
}

void fingerEnd(InputDevice *indev, Finger *finger) {
}

void fingerIdle(InputDevice *indev, Finger *finger) {
    // nothing
}

// 4.1 Call From finger*, Gesture relate
// ----------------------------------------
void GestureReset(Gesture *gesture) {
    gesture->direction = DIR_NONE;
    gesture->sum.x = 0;
    gesture->sum.y = 0;
}

void GestureStart(Finger *finger, Gesture* gesture) {
    GestureReset(gesture);
}

void GestureActive(Finger *finger, Gesture* gesture) {
    if(gesture->direction != DIR_NONE) { return; }

    Point diff = {
        finger->touch_point.point.x - finger->prev_point.x,
        finger->touch_point.point.y - finger->prev_point.y
    };

    #define MIN_VELOCITY 3
    if ( LV_ABS(diff.x) < MIN_VELOCITY && LV_ABS(diff.y) < MIN_VELOCITY ) {
        GestureReset(gesture);
        return;
    }

    gesture->sum.x += diff.x;
    gesture->sum.y += diff.y;

    #define GESTURE_LIMIT 50
    if (LV_ABS(gesture->sum.x) > GESTURE_LIMIT ||
        LV_ABS(gesture->sum.y) > GESTURE_LIMIT)
    {
        if(LV_ABS(gesture->sum.x) > LV_ABS(gesture->sum.y)) {
            if(gesture->sum.x > 0) {
                gesture->direction = DIR_RIGHT;
            } else {
                gesture->direction = DIR_LEFT;
            }
        } else {
            if(gesture->sum.y > 0) {
                gesture->direction = DIR_BOTTOM;
            } else {
                gesture->direction = DIR_TOP;
            }
        }
        printf("Direction %d\n", gesture->direction);
    }
}

// 4.1 Call From finger*, LongPress relate
// ----------------------------------------
void LongPressReset(LongPress *longPress) {
    longPress->isLong = 0;
    longPress->startTick = xTaskGetTickCount();
}

void LongPressStart(Finger *finger, LongPress *longPress) {
    LongPressReset(longPress);
}

#define LONGPRESS_THRESHOLD pdMS_TO_TICKS(1000)
void LongPressActive(Finger *finger, LongPress *longPress) {
    if(longPress->isLong) { return; }
    if (xTaskGetTickCount() - longPress->startTick > LONGPRESS_THRESHOLD) {
        longPress->isLong = 1;
        printf("LongPress\n");
    }

}

// ========================================
// Adapter
// ========================================
void InputDeviceScanCore(InputDevice* indev, ScanData *data) {
    data->touch_points[0].point.x = *TOUCH_X;
    data->touch_points[0].point.y = *TOUCH_Y;
    if (*TOUCH_PRESS) {
        data->slot_mask |= (1 << 0);
        data->touch_points[0].track_id = 1;
    } else {
        data->slot_mask &= ~(1 << 0);
        data->touch_points[0].track_id = -1;
    }
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
const uint32_t PERIOD = 33; // ms
static TimerHandle_t touch_timer = NULL;
void touch_timer_callback(TimerHandle_t xTimer) {
    printf("Tick count: %u\n ", xTaskGetTickCount());
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
    xTimerStart(touch_timer, portMAX_DELAY);

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
