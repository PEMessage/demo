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
#include <assert.h>

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

#define ARRAY_INDEX(ptr, array) ((size_t)((void *)(ptr) - (void *)(array)) / sizeof((array)[0]))
// ========================================
// Input Framework Helper
// ========================================
typedef uint16_t ObjectID;


// ========================================
// Input Framework Struct
// ========================================

// 1. Basic
// ----------------------------------------
typedef struct Point {
    int16_t x;
    int16_t y;
} Point;


typedef struct TouchPoint {
    int32_t track_id;
    Point point;
} TouchPoint;




// 2. Finger
// ----------------------------------------
// ST_NONE -(meet condition)-> ST_ONGOING -(loop eval)--- (meet condition) -----> ST_RECOGNIZED (do not eval anymore)
//                                                     \- (meet cancel condition)-> ST_CANCEL (do not eval anymore)
//

typedef enum State {
    ST_NONE = 0,
    ST_ONGOING = 1,
    ST_RECOGNIZED = 2,
    ST_CANCEL = 3
} State;

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
    State state;
    Direction direction;
    Point sum;
} Gesture;

typedef struct LongPress {
    State state;
    TickType_t startTick;
} LongPress;

typedef struct Click {
    State state;
    uint8_t count;
    TickType_t watchdog;
} Click;

typedef struct Finger {
    TouchPoint touch_point;


    Point start_point;
    Point prev_point;

    /* event base private data */
    Gesture gesture;
    LongPress longPress;
    Click click;
} Finger;

#define MULTIGESTURE_MAX_LIMIT 2
typedef struct MultiGesture {
    State state;
    TickType_t watchdog;
    uint8_t index;
    Finger *pipe[MULTIGESTURE_MAX_LIMIT];
} MultiGesture;

#define MAX_SUPPORT_SLOT 3

typedef struct ScanData {
    uint32_t slot_mask;
    TickType_t tick;
    TouchPoint touch_points[MAX_SUPPORT_SLOT];
} ScanData;


typedef struct InputDevice {
    bool enabled : 1;

    uint32_t prev_slot_mask;
    uint32_t curr_slot_mask;

    TickType_t tick;
    Finger fingers[MAX_SUPPORT_SLOT];

    /* event base private data */
    MultiGesture multigesture;
} InputDevice;

struct InputDevice *DEV;


// ========================================
// Input Framework Function
// ========================================
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
            for (int i = 0 ; i < MAX_SUPPORT_SLOT ; i ++ ) {
                data.touch_points[i] = indev->fingers[i].touch_point;
            }
            data.slot_mask = indev->curr_slot_mask;
            data.tick = 0;
        }

        void InputDeviceScanCore(InputDevice* indev, ScanData *data);
        InputDeviceScanCore(indev, &data);

        {
            if (data.tick == 0) { indev->tick = xTaskGetTickCount(); };
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
    uint32_t slot_mask_high = ~slot_mask_both_edge & indev->curr_slot_mask;
    uint32_t slot_mask_low = ~slot_mask_both_edge & ~indev->curr_slot_mask;

    FOREACH_SETBITS(slot_mask_up_edge, i) {
        void fingerStart(InputDevice *indev, Finger *finger);
        fingerStart(indev, &indev->fingers[i]);
    }

    FOREACH_SETBITS(slot_mask_down_edge, i) {
        void fingerEnd(InputDevice *indev, Finger *finger);
        fingerEnd(indev, &indev->fingers[i]);
    }

    FOREACH_SETBITS(slot_mask_high, i) {
        void fingerActive(InputDevice *indev, Finger *finger);
        fingerActive(indev, &indev->fingers[i]);
    }

    FOREACH_SETBITS(slot_mask_low, i) {
        void fingerIdle(InputDevice *indev, Finger *finger);
        fingerIdle(indev, &indev->fingers[i]);
    }

}


// 3.1 Call from detectProcess, Finger relate
// ----------------------------------------
void fingerStart(InputDevice *indev, Finger *finger) {
    finger->start_point = finger->touch_point.point;
    finger->prev_point = finger->touch_point.point;

    void GestureStart(InputDevice *indev, Finger *finger);
    GestureStart(indev, finger);

    void LongPressStart(InputDevice *indev, Finger *finger);
    LongPressStart(indev, finger);
}

void fingerActive(InputDevice *indev, Finger *finger) {
    void GestureActive(InputDevice *indev, Finger *finger);
    GestureActive(indev, finger);

    void MultiGestureActive(InputDevice *indev, Finger *finger);
    MultiGestureActive(indev, finger);

    void LongPressActive(InputDevice *indev, Finger *finger);
    LongPressActive(indev, finger);


    // Keep this at the end
    finger->prev_point = finger->touch_point.point;
}

void fingerEnd(InputDevice *indev, Finger *finger) {
    void MultiGestureEnd(InputDevice *indev, Finger *finger);
    MultiGestureEnd(indev, finger);

    void ClickStart(InputDevice *indev, Finger *finger);
    ClickStart(indev, finger);
}

void fingerIdle(InputDevice *indev, Finger *finger) {
    void ClickActive(InputDevice *indev, Finger *finger);
    ClickActive(indev, finger);
}

// 4.1 Call From finger*, Gesture relate
// ----------------------------------------
void GestureReset(InputDevice *indev, Finger *finger) {
    Gesture *gesture = &finger->gesture;
    gesture->direction = DIR_NONE;
    gesture->sum.x = 0;
    gesture->sum.y = 0;

    gesture->state = ST_NONE;
}

void GestureStart(InputDevice *indev, Finger *finger) {
    GestureReset(indev, finger);
    Gesture *gesture = &finger->gesture;

    gesture->state = ST_ONGOING;
}

void GestureActive(InputDevice *indev, Finger *finger) {
    Gesture *gesture = &finger->gesture;
    if(gesture->state != ST_ONGOING) { return; }

    Point diff = {
        finger->touch_point.point.x - finger->prev_point.x,
        finger->touch_point.point.y - finger->prev_point.y
    };

    #define MIN_VELOCITY 3
    if ( LV_ABS(diff.x) < MIN_VELOCITY && LV_ABS(diff.y) < MIN_VELOCITY ) {
        GestureReset(indev, finger);
        gesture->state = ST_ONGOING; // clean Gesture state, but still perform eval in same finger
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
        gesture->state = ST_RECOGNIZED;
        printf("[EV %d]: Direction %d\n",
                ARRAY_INDEX(finger, indev->fingers),
                gesture->direction
              );

        void MultiGestureStart(InputDevice *indev, Finger *finger);
        MultiGestureStart(indev, finger);
    }
}

// 4.2 Call From finger*, LongPress relate
// ----------------------------------------
void LongPressReset(InputDevice *indev, Finger *finger) {
    LongPress *longPress = &finger->longPress;
    longPress->startTick = indev->tick;

    longPress->state = ST_NONE;
}

void LongPressStart(InputDevice *indev, Finger *finger) {
    LongPressReset(indev, finger);

    LongPress *longPress = &finger->longPress;
    longPress->state = ST_ONGOING;

}

#define LONGPRESS_THRESHOLD pdMS_TO_TICKS(1000)
void LongPressActive(InputDevice *indev, Finger *finger) {
    LongPress *longPress = &finger->longPress;
    if(longPress->state != ST_ONGOING ) { return; }

    if (indev->tick - longPress->startTick > LONGPRESS_THRESHOLD) {
        longPress->state = ST_RECOGNIZED;
        printf("[EV %d]: LongPress\n",
                ARRAY_INDEX(finger, indev->fingers)
              );
    }

}

// 4.3 Call From finger*, Click relate
// ----------------------------------------
// ST_NONE <-> count == 0
// ST_ONGOING <-> count != 0
void ClickAssert(InputDevice *indev, Finger *finger) {
    assert(indev);
    assert(finger);
    assert( \
            (finger->click.state == ST_NONE && finger->click.count == 0) ||
            (finger->click.state == ST_ONGOING && finger->click.count != 0) ||
            (
             finger->click.state == ST_CANCEL ||
             finger->click.state == ST_RECOGNIZED
            ) );

}
void ClickReset(InputDevice *indev, Finger *finger) {
    Click *click = &finger->click;
    ClickAssert(indev, finger);

    click->count = 0;
    click->state = ST_NONE;

    ClickAssert(indev, finger);
}

#define MULTICLICK_MAX_CLICK 2
void ClickStart(InputDevice *indev, Finger *finger) {
    Click *click = &finger->click;
    ClickAssert(indev, finger);
    switch(click->state) {
        case ST_NONE:
            click->count = 1;
            click->watchdog = indev->tick; // init watchdog
            click->state = ST_ONGOING;
            break;
        case ST_ONGOING:
            click->count++;
            click->watchdog = indev->tick; // feed the dog

            #if defined(MULTICLICK_MAX_CLICK) && MULTICLICK_MAX_CLICK > 0
            if (click->count == MULTICLICK_MAX_CLICK) {
                click->state = ST_RECOGNIZED;
                printf("[EV %d]: [L] Click %d\n",
                        ARRAY_INDEX(finger, indev->fingers),
                        click->count
                      );
                ClickReset(indev, finger);
            }
            #endif
            break;
        default:
            assert(0);
            return;

    }
}


#define MULTICLICK_THRESHOLD pdMS_TO_TICKS(100)
void ClickActive(InputDevice *indev, Finger *finger) {
    Click *click = &finger->click;
    if(click->state != ST_ONGOING ) { return; }

    if (finger->longPress.state == ST_RECOGNIZED) {
        click->state = ST_RECOGNIZED;
        printf("[EV %d]: LongClick\n",
                ARRAY_INDEX(finger, indev->fingers)
              );
        ClickReset(indev, finger);
        return;
    }

    if (finger->gesture.state == ST_RECOGNIZED) {
        click->state = ST_CANCEL;
        ClickReset(indev, finger);
        return;
    }

    #if defined(MULTICLICK_MAX_CLICK) && MULTICLICK_MAX_CLICK > 0
    if (click->count == MULTICLICK_MAX_CLICK) {
        click->state = ST_RECOGNIZED;
        printf("[EV %d]: [L] Click %d\n",
                ARRAY_INDEX(finger, indev->fingers),
                click->count
              );
        ClickReset(indev, finger);
        return;
    }
    #endif


    // dog timeout
    if (indev->tick - click->watchdog > MULTICLICK_THRESHOLD) {
        click->state = ST_RECOGNIZED;
        printf("[EV %d]: [W] Click %d\n",
                ARRAY_INDEX(finger, indev->fingers),
                click->count
              );
        ClickReset(indev, finger);
        return;
    }

    return;
}

// 5.1 Call From Gesture*, MultiGesture relate
// ----------------------------------------
// TODO: currently we simplely clean all fifo
//          1. if full
//          2. watdog bark
//          3. any finger lift
//       we should add gesture detect
void MultiGestureReset(InputDevice *indev, Finger *finger) {
    MultiGesture *mg = &indev->multigesture;

    for (int i = 0; i < mg->index ; i++) {
        printf("[EV %d]: [*] Direction %d\n",
                ARRAY_INDEX(mg->pipe[i], indev->fingers),
                mg->pipe[i]->gesture.direction
              );
    }
    mg->index = 0;
    mg->state = ST_NONE;
}

void MultiGestureStart(InputDevice *indev, Finger *finger) {
    MultiGesture *mg = &indev->multigesture;
    switch(mg->state) {
        case ST_NONE:
        case ST_ONGOING:
            mg->watchdog = indev->tick; // init or feed the dog
            mg->pipe[mg->index++] = finger; // push to fifo
            mg->state = ST_ONGOING;
            break;
        default:
            assert(0);
    }

    if (mg->index == MULTIGESTURE_MAX_LIMIT) {
        MultiGestureReset(indev, finger);
    }
}

void MultiGestureEnd(InputDevice *indev, Finger *finger) {
    MultiGestureReset(indev, finger);
}

#define MULTIGESTURE_THRESHOLD pdMS_TO_TICKS(100)
void MultiGestureActive(InputDevice *indev, Finger *finger) {
    MultiGesture *mg = &indev->multigesture;
    if(mg->state != ST_ONGOING) { return; }

    // watchdog overtime
    if(indev->tick - mg->watchdog > MULTIGESTURE_THRESHOLD ) {
        MultiGestureReset(indev, finger);
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
const uint32_t PERIOD = 100; // ms
static TimerHandle_t touch_timer = NULL;
void touch_timer_callback(TimerHandle_t xTimer) {
    // printf("Tick count: %u\n ", xTaskGetTickCount());
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
