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
#include "assert_static.h"

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
// Helper Function or Macro
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

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define ASSERT_CREATE_MASK(n)  ASSERT_STATIC((n)>=0 && (n) < 63, "Error")
#define CREATE_MASK(n)         ((1ULL << ((n) + 1)) - 1)

// ========================================
// Coroutine
// ========================================
/*
 * This allows us to write functions that "pause" and "resume".
 * It uses the line number (__LINE__) as the state ID.
 */

#define CR_FIELD \
    int line; \
    TickType_t timer_start

typedef struct {
    CR_FIELD;
} cr_ctx_t;

#define CR_START(ctx)     switch((ctx)->line) { case 0:
#define CR_YIELD(ctx)     do { (ctx)->line = __LINE__; return; case __LINE__:; } while(0)
#define CR_RESET(ctx)     do { (ctx)->line = 0; } while(0)
#define CR_END(ctx)       } (ctx)->line = 0;

/* Helpers for timing and waiting */
#define CR_AWAIT(ctx, cond) \
    while(!(cond)) { CR_YIELD(ctx); }

#define CR_DELAY(ctx, tick_now, duration) \
    (ctx)->timer_start = (tick_now); \
    while(((tick_now) - (ctx)->timer_start) < (duration)) { CR_YIELD(ctx); }


// ========================================
// Input Framework Struct
// ========================================

// 1. Basic
// ----------------------------------------
typedef struct Point {
    int16_t x;
    int16_t y;
} Point;

#define TOUCHPOINT_FIELD \
    int32_t track_id; \
    Point point


typedef struct TouchPoint {
    TOUCHPOINT_FIELD;
} TouchPoint;


// 2. Finger
// ----------------------------------------
// ST_NONE -(meet condition)-> ST_ONGOING -(loop eval)--- (meet recoginze condition) -----> ST_RECOGNIZED (do not eval anymore)
//                                                     \- (meet cancel condition) --------> ST_CANCEL (do not eval anymore)
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

#define MAX_SUPPORT_SLOT 3

typedef struct ScanData {
    uint32_t slot_mask;
    TickType_t tick;
    TouchPoint touch_points[MAX_SUPPORT_SLOT];
} ScanData;

// Common data might be used for other coroutine
typedef struct {
    CR_FIELD;

    Point start_point;
    Point end_point;
} cr_common_t;

typedef struct {
    CR_FIELD;

    // internal value
    Point prev;
    Point sum;

    // result
    Direction direction;
} cr_gesture_t;

typedef struct {
    CR_FIELD;

    TickType_t timer;
} cr_longpress_t;

typedef struct {
    CR_FIELD;

    int count;
    TickType_t watchdog;

    uint8_t is_long :1;
    uint8_t is_gesture :1;
} cr_click_t;

typedef struct Finger {
    TOUCHPOINT_FIELD;

    uint8_t is_active : 1;
    uint8_t is_edge : 1;

    cr_common_t cr_common;
    cr_gesture_t cr_gesture;
    cr_longpress_t cr_longpress;
    cr_click_t cr_click;
} Finger;

#define MULTIGESTURE_MAX 3
typedef struct {
    CR_FIELD;

    TickType_t watchdog;
    uint8_t end;

    uint32_t fifo_mask;
    Finger *fifo[MULTIGESTURE_MAX];

    Direction direction;
} cr_multigesture_t;

typedef struct InputDevice {
    bool enabled : 1;

    uint32_t prev_slot_mask;
    uint32_t curr_slot_mask;

    TickType_t tick;


    Finger fingers[MAX_SUPPORT_SLOT];

    cr_multigesture_t cr_multigesture;
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


    { // Actually Read something
        struct ScanData data;
        {
            for (int i = 0 ; i < MAX_SUPPORT_SLOT ; i ++ ) {
                data.touch_points[i].point = indev->fingers[i].point;
                data.touch_points[i].track_id = indev->fingers[i].track_id;
            }
            data.slot_mask = indev->curr_slot_mask;
            data.tick = 0;
        }

        void InputDeviceScanCore(InputDevice* indev, ScanData *data);
        InputDeviceScanCore(indev, &data);

        {
            if (data.tick == 0) {
                indev->tick = xTaskGetTickCount();
            } else {
                indev->tick = data.tick;
            }

            indev->prev_slot_mask = indev->curr_slot_mask;
            indev->curr_slot_mask = data.slot_mask;

            const uint32_t slot_mask_both_edge = indev->prev_slot_mask ^ indev->curr_slot_mask;

            for (int i = 0 ; i < MAX_SUPPORT_SLOT ; i ++ ) {
                indev->fingers[i].track_id = data.touch_points[i].track_id;
                indev->fingers[i].point = data.touch_points[i].point;

                // helper field relate from other indev->*mask
                const uint32_t mask = (1 << i);
                indev->fingers[i].is_active = (bool)(mask & indev->curr_slot_mask);
                indev->fingers[i].is_edge = (bool)(mask & slot_mask_both_edge);
            }
        }
    }

    void detectProcess(InputDevice* indev);
    detectProcess(indev);
}

// 2. Call from InputDeviceScan
// ----------------------------------------

void detectProcess(InputDevice* indev) {
    for(int i = 0; i < MAX_SUPPORT_SLOT ; i++) {
        void Co_Common(InputDevice *indev, Finger *f);
        Co_Common(indev, &indev->fingers[i]);

        void Co_Gesture(InputDevice *indev, Finger *f);
        Co_Gesture(indev, &indev->fingers[i]);

        void Co_LongPress(InputDevice *indev, Finger *f);
        Co_LongPress(indev, &indev->fingers[i]);

        void Co_Click(InputDevice *indev, Finger *f);
        Co_Click(indev, &indev->fingers[i]);
    }
    void Co_MultiGesture(InputDevice *indev, Finger *f);
    Co_MultiGesture(indev, NULL);
}

// 3.0 Call from detectProcess, Common Relate
// ----------------------------------------

void Co_Common(InputDevice *indev, Finger *f) {
    cr_common_t * const cr_common = &f->cr_common;
    CR_START(cr_common);
    while(1) {
        CR_AWAIT(cr_common, f->is_active && f->is_edge);
        cr_common->start_point = f->point;
        cr_common->end_point = f->point;
        printf("[EV %d]: start x %d, y %d\n",
                ARRAY_INDEX(f, indev->fingers),
                cr_common->start_point.x, cr_common->start_point.y
                );
        CR_YIELD(cr_common);
        while(1) {
            if (!f->is_active) {
                printf("[EV %d]: end x %d, y %d\n",
                        ARRAY_INDEX(f, indev->fingers),
                        cr_common->start_point.x, cr_common->start_point.y
                      );
                CR_RESET(cr_common);
                return;
            }
            cr_common->end_point = f->point;
            CR_YIELD(cr_common);
        }
    }
    CR_END(cr_common);
}

// 3.1 Call from detectProcess, Gesture Relate
// ----------------------------------------

#define GESTURE_LIMIT 50
#define MIN_VELOCITY 3


void GestureReset(InputDevice *indev, Finger *f) {
    cr_gesture_t * const cr_gesture = &f->cr_gesture;
    cr_gesture->sum.x = 0;
    cr_gesture->sum.y = 0;
    cr_gesture->prev = f->point;
    cr_gesture->direction = DIR_NONE;
}

void Co_Gesture(InputDevice *indev, Finger *f) {
    cr_gesture_t * const cr_gesture = &f->cr_gesture;

    CR_START(&f->cr_gesture);
    while(1) {
        CR_AWAIT(cr_gesture, f->is_active && f->is_edge);

        GestureReset(indev, f);
        CR_YIELD(cr_gesture);

        while(1) {
            // equal to `f->is_active || (!f->is_active && f->is_edge)`
            CR_AWAIT(cr_gesture, f->is_active || f->is_edge);

            // This part handle signal: `___/\___` (up and down real quick)
            // If could ensure that signal will not change that fast,
            // something like `___/-\___`, we don't need this part
            if (f->is_edge) {
                // NOTE: Cancel: execute flow but do not reset closure
                CR_RESET(cr_gesture);
                return;
            }

            Point diff = {
                .x = f->point.x - cr_gesture->prev.x,
                .y = f->point.y - cr_gesture->prev.y,
            };

            if ( LV_ABS(diff.x) < MIN_VELOCITY && LV_ABS(diff.y) < MIN_VELOCITY ) {
                // NOTE: keep at ONGOING, but reset all closure
                GestureReset(indev, f);
                CR_YIELD(cr_gesture);
                continue;
            }

            cr_gesture->sum.x += diff.x;
            cr_gesture->sum.y += diff.y;

            if (LV_ABS(cr_gesture->sum.x) > GESTURE_LIMIT || LV_ABS(cr_gesture->sum.y) > GESTURE_LIMIT) {
                if (LV_ABS(cr_gesture->sum.x) > LV_ABS(cr_gesture->sum.y)) {
                    cr_gesture->direction = (cr_gesture->sum.x > 0) ? DIR_RIGHT : DIR_LEFT;
                } else {
                    cr_gesture->direction = (cr_gesture->sum.y > 0) ? DIR_BOTTOM : DIR_TOP;
                }
                // NOTE: RECOGINZE, (so CR_RESET return is stop eval)
                // printf("[EV %d]: Direction %d\n",
                //         ARRAY_INDEX(f, indev->fingers),
                //         cr_gesture->direction
                //       );
                void SendToMultiGesture(InputDevice *indev, Finger *f);
                SendToMultiGesture(indev, f);
                void NotifyClickGesture(InputDevice *indev, Finger *f);
                NotifyClickGesture(indev, f);
                CR_RESET(cr_gesture);
                return;
            }
            cr_gesture->prev = f->point;
            CR_YIELD(cr_gesture);
            continue;
        }

    }
    CR_END(cr_gesture)
}

// 3.2 Call from detectProcess, LongPress
// ----------------------------------------
#define LONGPRESS_THRESHOLD pdMS_TO_TICKS(500)
void Co_LongPress(InputDevice *indev, Finger *f) {
    cr_longpress_t * const cr_longpress = &f->cr_longpress;
    CR_START(cr_longpress);
    while(1) {

        CR_AWAIT(cr_longpress, f->is_active && f->is_edge);
        cr_longpress->timer = indev->tick;
        CR_YIELD(cr_longpress);

        while(1) {
            CR_AWAIT(cr_longpress, f->is_active || f->is_edge);

            if (f->is_edge) {
                CR_RESET(cr_longpress);
                return;
            }

            if (!(indev->tick - cr_longpress->timer > LONGPRESS_THRESHOLD)) {
                CR_YIELD(cr_longpress);
                continue;
            }

            printf("[EV %d]: LongPress\n",
                    ARRAY_INDEX(f, indev->fingers)
                  );
            void NotifyClickLong(InputDevice *indev, Finger *f);
            NotifyClickLong(indev, f);

            CR_RESET(cr_longpress);
            return;
        }
    }
    CR_END(cr_longpress);
}

// 3.2 Call from detectProcess, Click
// ----------------------------------------
#define CLICK_MAX 3
#define CLICK_THRESHOLD pdMS_TO_TICKS(300)

void NotifyClickLong(InputDevice *indev, Finger *f) {
    f->cr_click.is_long = 1;
}

void NotifyClickGesture(InputDevice *indev, Finger *f) {
    f->cr_click.is_gesture = 1;
}

void ClickReset(InputDevice *indev, Finger *f) {
    cr_click_t * const click = &f->cr_click;
    const int is_special = (click->is_long || click->is_gesture);
    const int normal_count = click->count - is_special;

    assert(normal_count >= 0);

    if (normal_count != 0) {
        printf("[EV %d]: [%c] Click %d\n",
                ARRAY_INDEX(f, indev->fingers),
                (normal_count == CLICK_MAX) ? 'M' : 'W',
                normal_count
              );
    }

    if (click->is_long) {
        printf("[EV %d]: LongClick\n",
                ARRAY_INDEX(f, indev->fingers)
              );
    }

    if (click->is_gesture) {
        // do nothing
    }

    click->count = 0;
    click->is_long = 0;
    click->is_gesture = 0;
}

void Co_Click(InputDevice *indev, Finger *f) {
    cr_click_t * const cr_click = &f->cr_click;
    const int is_special = (cr_click->is_long || cr_click->is_gesture);
    CR_START(cr_click);
    while(1) {
        CR_AWAIT(cr_click, !f->is_active && f->is_edge);


        cr_click->count = 1;
        cr_click->watchdog = indev->tick;

        if (is_special) {
            ClickReset(indev, f);
            CR_RESET(cr_click);
            return;
        } else {
            CR_YIELD(cr_click);
        }


        while(1) {
            CR_AWAIT(cr_click,
                    (!f->is_active && f->is_edge) ||
                    (indev->tick - cr_click->watchdog > CLICK_THRESHOLD)
                    )

            if (!f->is_active && f->is_edge) {
                cr_click->count++;
                cr_click->watchdog = indev->tick;

                if (cr_click->count != CLICK_MAX && !is_special) {
                    CR_YIELD(cr_click);
                    continue;
                }
            }

            ClickReset(indev, f);
            CR_RESET(cr_click);
            return;
        }
    }
    CR_END(cr_click)
}

// 4.1 Call from detectProcess, MultiGesture
// ----------------------------------------

// Notice: fingers[] is not same order as `indev->fingers[]`
void MultiGestureDetect2Finger(InputDevice *indev, Finger *fingers[]) {
    cr_multigesture_t *mg = &indev->cr_multigesture;

    assert(fingers[0]->cr_gesture.direction != DIR_NONE);
    assert(fingers[1]->cr_gesture.direction != DIR_NONE);

    if (fingers[0]->cr_gesture.direction == fingers[1]->cr_gesture.direction) {
        mg->direction = fingers[0]->cr_gesture.direction;
    }

}

void MultiGestureDetect3Finger(InputDevice *indev, Finger *fingers[]) {
    cr_multigesture_t *mg = &indev->cr_multigesture;

    assert(fingers[0]->cr_gesture.direction != DIR_NONE);
    assert(fingers[1]->cr_gesture.direction != DIR_NONE);
    assert(fingers[2]->cr_gesture.direction != DIR_NONE);

    if (
        fingers[0]->cr_gesture.direction == fingers[1]->cr_gesture.direction &&
        fingers[1]->cr_gesture.direction == fingers[2]->cr_gesture.direction
       ) {
        mg->direction = fingers[0]->cr_gesture.direction;
    }

}


// Check order N -> N-1 -> ... -> 3 -> 2 -> 1
void (*MULTIGESTURE_DETECTFUNC[])(InputDevice *indev, Finger *fingers[]) = {
    NULL, // `1 FingerDetectfunc` do not exist, we already know it in finger->gesture.direction
    MultiGestureDetect2Finger,
    MultiGestureDetect3Finger,
};
ASSERT_STATIC(ARRAY_SIZE(MULTIGESTURE_DETECTFUNC) == MULTIGESTURE_MAX, "Out of Sync");


void MultiGestureReset(InputDevice *indev, Finger *finger) {
    cr_multigesture_t *mg = &indev->cr_multigesture;

    char *source = NULL;
    if (finger == NULL) {
        source = "[W]";
    } else if ( (1 << ARRAY_INDEX(finger, indev->fingers)) & indev->curr_slot_mask) {
        source = "[F]";
    } else {
        source = "[L]";
    }

    // 1. Found if any gesture meet
    //  [0 ... [      i      ) ... end)
    //  \______|_____________/        |  : Send Raw Gesture
    //         \______________________/  : Send Multi Gesture
    //
    // Iter all `N FingerDetectfunc` if `N <= len(fifo)`
    // i == 0 <-> 1 FingerDetectfunc
    // i == 1 <-> 2 FingerDetectfunc
    // ...
    //
    int i;
    for (i = 0; i < mg->end; i ++) {
        const int finger_number =  mg->end - i; // current len(mg->fifo)
        assert(finger_number > 0);
        const int detectfunc_index = finger_number - 1;

        if(MULTIGESTURE_DETECTFUNC[detectfunc_index] == NULL) { continue; }

        // 1. Check any know gesture combination exist in fifo
        Finger **begin = &mg->fifo[i];
        MULTIGESTURE_DETECTFUNC[detectfunc_index](indev, begin);
        if (mg->direction == DIR_NONE) {
            continue;  // pervious call do not output anything, check next
        } else {
            break;
        }
    }


    // 2. if any `gesture combination` exsit, dequeue fifo content until only gesture combination exist
    for (int j = 0; j < i; j++) {
        printf("[EV %d]: %s Direction %d\n",
                ARRAY_INDEX(mg->fifo[j], indev->fingers),
                source,
                mg->fifo[j]->cr_gesture.direction
              );
    }

    // 3. Send `gesture combination` instead of single gesture
    // TODO: it's fine for now, since we only have one kind of multigesture
    //       change it if we add more
    if(i != mg->end) {
        const int finger_number =  mg->end - i; // current len(mg->fifo)
        printf("[EV M%d]: %s Direction %d\n",
                finger_number,
                source,
                mg->direction
              );
    }

    mg->end = 0;
    mg->fifo_mask = 0;
}

void SendToMultiGesture(InputDevice *indev, Finger *f) {
    cr_multigesture_t * const mg = &indev->cr_multigesture;
    mg->watchdog = indev->tick;
    mg->fifo_mask |= 1 << ARRAY_INDEX(f, indev->fingers);
    mg->fifo[mg->end++] = f;
    if (mg->end == MULTIGESTURE_MAX) {
        MultiGestureReset(indev, f);
    }
}

#define MULTIGESTURE_THRESHOLD pdMS_TO_TICKS(200)
void Co_MultiGesture(InputDevice *indev, Finger *f) {
    cr_multigesture_t * const cr_mg = &indev->cr_multigesture;
    CR_START(cr_mg);
    while(1) {
        // watchdog bark or any finger which already detect gesture lift
        CR_AWAIT(cr_mg,
                (cr_mg->end != 0 && indev->tick - cr_mg->watchdog > MULTIGESTURE_THRESHOLD) ||
                (cr_mg->end != 0 && ((cr_mg->fifo_mask & indev->curr_slot_mask) != cr_mg->fifo_mask))
                )
        MultiGestureReset(indev, f);
        CR_RESET(cr_mg);
        return;
    }
    CR_END(cr_mg)

}


// ========================================
// Adapter
// ========================================
void InputDeviceScanCore(InputDevice* indev, ScanData *data) {
    ASSERT_CREATE_MASK(MAX_SUPPORT_SLOT - 1);
    static const uint32_t SUPPORT_MASK = CREATE_MASK(MAX_SUPPORT_SLOT - 1);

    data->slot_mask = MPS2FB_TOUCH->header.points_mask & SUPPORT_MASK;
    // Mock 2/3 finger
    // data->slot_mask |= (data->slot_mask & 1) << 1;
    // data->slot_mask |= (data->slot_mask & 1) << 2;

    for (int i = 0 ; i < MAX_SUPPORT_SLOT ; i ++) {
        // Mock 2/3 finger
        // if (i == 1 || i == 2) {
        //     data->touch_points[i].point.x = MPS2FB_TOUCH->points[0].x + 10 * i;
        //     data->touch_points[i].point.y = MPS2FB_TOUCH->points[0].y + 10 * i;
        //     data->touch_points[i].track_id = i + 1;
        //     continue;
        // }
        data->touch_points[i].point.x = MPS2FB_TOUCH->points[i].x;
        data->touch_points[i].point.y = MPS2FB_TOUCH->points[i].y;
        data->touch_points[i].track_id = MPS2FB_TOUCH->points[i].track_id;
    }

    return ;
}

// Call From IRQ
// ----------------------------------------
// Task handle for touch processing
static TaskHandle_t touch_task_handle = NULL;

// Touch task function
#define STATISTICS_PERIOD 5000
void touch_task(void *pvParameters) {

    struct InputDevice *indev = (struct InputDevice *)pvParameters;

    #if defined(STATISTICS_PERIOD) && STATISTICS_PERIOD > 0
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint32_t notify_count = 0;
    #endif

    while(1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        InputDeviceScan(indev);

        #if defined(STATISTICS_PERIOD) && STATISTICS_PERIOD > 0
        notify_count++;
        if ((xTaskGetTickCount() - xLastWakeTime) > pdMS_TO_TICKS(STATISTICS_PERIOD)) {
            // Get high water mark
            UBaseType_t high_water_mark = uxTaskGetStackHighWaterMark(NULL);

            // Print statistics
            printf("Statistics: HighWaterMark %lu Byte, Rate %d\n", high_water_mark * sizeof(StackType_t), notify_count / STATISTICS_PERIOD);

            notify_count = 0;
            xLastWakeTime = xTaskGetTickCount();
        }
        #endif
    }
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

    // Send direct notification to wake up touch task
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (touch_task_handle != NULL) {
        vTaskNotifyGiveFromISR(touch_task_handle, &xHigherPriorityTaskWoken);
    }

    // Yield if necessary
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


// Call From Timer
// ----------------------------------------
const uint32_t PERIOD = 100; // ms
static TimerHandle_t touch_timer = NULL;
void touch_timer_callback(TimerHandle_t xTimer) {
    // printf("Tick count: %u\n ", xTaskGetTickCount());
    if (touch_task_handle) {
        xTaskNotifyGive(touch_task_handle);
    }
}


// ========================================
// Main Task
// ========================================

void fill_framebuffer() {
    volatile uint32_t *fb_ptr = FB_BASE_ADDRESS;
    uint32_t y, x; // Use x_byte for iterating through bytes horizontally

    for (y = 0; y < FB_HEIGHT; ++y) {

        for (x = 0; x < FB_WIDTH; ++x) {
            fb_ptr[x + y * FB_WIDTH] = 0xFFFFFFFF;
        }
    }
}

void MainTask() {
    // create indev
    DEV = InputDeviceCreate();
    fill_framebuffer();

    // 1. Create touch processing task
    if (xTaskCreate(
            touch_task,
            "TouchTask",
            configMINIMAL_STACK_SIZE + 512 / sizeof(StackType_t),  // Adjust stack size as needed
            (void *)DEV,                      // Pass input device as parameter
            (tskIDLE_PRIORITY + 2),           // Higher priority than main task
            &touch_task_handle
        ) != pdPASS) {
        printf("Failed to create touch task\n");
        return;
    }
    printf("Touch task created successfully\n");

    // 2. Create a timer (initially not active)
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
        // Main task can do other work here
        vTaskDelay(pdMS_TO_TICKS(portMAX_DELAY));  // Example delay
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
