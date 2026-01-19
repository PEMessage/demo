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
#define LV_MIN(a, b) ((a) < (b) ? (a) : (b))
#define LV_MAX(a, b) ((a) > (b) ? (a) : (b))
#define LV_CLAMP(min, val, max) (LV_MAX(min, (LV_MIN(val, max))))

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
    int line

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

    uint8_t is_long :1;
    uint8_t is_gesture :1;
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
    uint32_t counter;

} cr_longpress_t;

typedef struct {
    CR_FIELD;

    int count;
    TickType_t watchdog;

    uint8_t is_special :1;

    Point point;
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

typedef struct {
    CR_FIELD;

    Finger *current;
    TickType_t timer;
    uint32_t counter;
} cr_select_t;

typedef struct {
    CR_FIELD;
    TickType_t timer;
} cr_select_and_confirm;

// Configuration structure for runtime settings
typedef struct InputDeviceConfig {
    // Gesture configuration
    int16_t gesture_limit;
    int16_t gesture_min_velocity;

    // Long press configuration
    TickType_t longpress_threshold;
    TickType_t longpress_update_interval;

    // Click configuration
    uint8_t click_max;
    TickType_t click_threshold;
    int16_t click_region_max;

    // Multi-gesture configuration
    TickType_t multigesture_threshold;
    uint8_t multigesture_max;

    // select configuration
    TickType_t select_update_interval;

} InputDeviceConfig;

typedef struct InputDevice {
    bool enabled : 1;

    uint32_t prev_slot_mask;
    uint32_t curr_slot_mask;

    TickType_t tick;

    // Configuration
    InputDeviceConfig config;

    Finger fingers[MAX_SUPPORT_SLOT];

    cr_multigesture_t cr_multigesture;
    cr_select_t cr_select;
} InputDevice;

struct InputDevice *DEV;

// ========================================
// Default Configuration Values
// ========================================
#define DEFAULT_GESTURE_LIMIT 50
#define DEFAULT_GESTURE_MIN_VELOCITY 3

#define DEFAULT_LONGPRESS_THRESHOLD pdMS_TO_TICKS(500)
#define DEFAULT_LONGPRESS_UPDATE_INTERVAL pdMS_TO_TICKS(100)

#define DEFAULT_CLICK_MAX 3
#define DEFAULT_CLICK_THRESHOLD pdMS_TO_TICKS(300)
#define DEFAULT_CLICK_REGION_MAX 25

#define DEFAULT_MULTIGESTURE_THRESHOLD pdMS_TO_TICKS(200)

#define DEFAULT_SELECT_UPDATE_INTERVAL pdMS_TO_TICKS(100)

#define DEFAULT_SELECT_AND_CONFIRM_UPDATE_INTERVAL pdMS_TO_TICKS(500)
#define DEFAULT_SELECT_AND_CONFIRM_THRESHOLD pdMS_TO_TICKS(2000)


// ========================================
// Input Framework Function
// ========================================

const InputDeviceConfig DEFAULT_INPUTDEVICE_CONFIG = {
    .gesture_limit = DEFAULT_GESTURE_LIMIT,
    .gesture_min_velocity = DEFAULT_GESTURE_MIN_VELOCITY,

    .longpress_threshold = DEFAULT_LONGPRESS_THRESHOLD,
    .longpress_update_interval = DEFAULT_LONGPRESS_UPDATE_INTERVAL,

    .click_max = DEFAULT_CLICK_MAX,
    .click_threshold = DEFAULT_CLICK_THRESHOLD,
    .click_region_max = DEFAULT_CLICK_REGION_MAX,

    .multigesture_threshold = DEFAULT_MULTIGESTURE_THRESHOLD,
    .multigesture_max = MULTIGESTURE_MAX,

    .select_update_interval = DEFAULT_SELECT_UPDATE_INTERVAL
};


// Backward compatibility function
InputDevice *InputDeviceCreate() {
    InputDevice *indev = malloc(sizeof(*indev));
    if (!indev) return NULL;

    memset(indev, 0, sizeof(*indev));

    // Set configuration
    indev->config = DEFAULT_INPUTDEVICE_CONFIG;

    return indev;
}

void InputDeviceEnable(InputDevice *indev) {
    indev->enabled = true;
}

void InputDeviceDisable(InputDevice *indev) {
    indev->enabled = false;
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

    void Co_Select(InputDevice *indev, Finger *f);
    Co_Select(indev, NULL);

    // allow provide global tick for user
    void OnScan(InputDevice *indev);
    OnScan(indev);

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

        cr_common->is_long = 0;
        cr_common->is_gesture = 0;

        // printf("[EV %d]: start x %d, y %d\n",
        //         ARRAY_INDEX(f, indev->fingers),
        //         cr_common->start_point.x, cr_common->start_point.y
        //         );
        CR_YIELD(cr_common);
        while(1) {
            if (!f->is_active) {
                // printf("[EV %d]: end x %d, y %d\n",
                //         ARRAY_INDEX(f, indev->fingers),
                //         cr_common->start_point.x, cr_common->start_point.y
                //       );
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

void GestureReset(InputDevice *indev, Finger *f) {
    cr_gesture_t * const cr_gesture = &f->cr_gesture;
    cr_gesture->sum.x = 0;
    cr_gesture->sum.y = 0;
    cr_gesture->prev = f->point;
    cr_gesture->direction = DIR_NONE;
}

void Co_Gesture(InputDevice *indev, Finger *f) {
    cr_gesture_t * const cr_gesture = &f->cr_gesture;
    cr_common_t * const common = &f->cr_common;

    CR_START(&f->cr_gesture);
    while(1) {
        CR_AWAIT(cr_gesture, f->is_active && f->is_edge);

        GestureReset(indev, f);
        CR_YIELD(cr_gesture);

        while(1) {
            // equal to `f->is_active || (!f->is_active && f->is_edge)`
            CR_AWAIT(cr_gesture, f->is_active || f->is_edge);

            // 1.
            // The `f->is_edge` handle signal: `___/\___` (up and down real quick)
            // If could ensure that signal will not change that fast,
            // something like `___/-\___`, we don't need to check f->is_edge
            if (f->is_edge) {
                // NOTE: Cancel: execute flow but do not reset closure
                CR_RESET(cr_gesture);
                return;
            }

            // 2.
            // gesture and longpress are mutually exclusive
            if (common->is_long) {
                CR_RESET(cr_gesture);
                return;
            }

            Point diff = {
                .x = f->point.x - cr_gesture->prev.x,
                .y = f->point.y - cr_gesture->prev.y,
            };

            if ( LV_ABS(diff.x) < indev->config.gesture_min_velocity &&
                 LV_ABS(diff.y) < indev->config.gesture_min_velocity ) {
                // NOTE: keep at ONGOING, but reset all closure
                GestureReset(indev, f);
                CR_YIELD(cr_gesture);
                continue;
            }

            cr_gesture->sum.x += diff.x;
            cr_gesture->sum.y += diff.y;

            if (LV_ABS(cr_gesture->sum.x) > indev->config.gesture_limit ||
                LV_ABS(cr_gesture->sum.y) > indev->config.gesture_limit) {
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
void Co_LongPress(InputDevice *indev, Finger *f) {
    cr_longpress_t * const cr_longpress = &f->cr_longpress;
    cr_common_t * const common = &f->cr_common;

    CR_START(cr_longpress);
    while(1) {

        CR_AWAIT(cr_longpress, f->is_active && f->is_edge);
        cr_longpress->timer = indev->tick;
        cr_longpress->counter = 0;
        CR_YIELD(cr_longpress);

        while(1) {
            CR_AWAIT(cr_longpress, f->is_active || f->is_edge);

            // 1.
            // avoid _/\_ like signal
            if (f->is_edge) {
                CR_RESET(cr_longpress);
                return;
            }

            // 2.
            // gesture and longpress are mutually exclusive
            if (common->is_gesture) {
                CR_RESET(cr_longpress);
                return;
            }

            if (!(indev->tick - cr_longpress->timer > indev->config.longpress_threshold)) {
                CR_YIELD(cr_longpress);
                continue;
            }

            printf("[EV %d]: LongPress initial\n",
                    ARRAY_INDEX(f, indev->fingers)
                  );
            void NotifyClickLong(InputDevice *indev, Finger *f);
            NotifyClickLong(indev, f);
            void NotifySelect(InputDevice *indev, Finger *f);
            NotifySelect(indev, f);

            /* uncomment for underLongPress event
            // below `cr_longpress->timer` reuse as update_interval timer
            cr_longpress->timer = indev->tick;
            CR_YIELD(cr_longpress);
            */
            while(1) {
                CR_AWAIT(cr_longpress,
                        (NotifySelect(indev, f), false)
                        || (!f->is_active)
                        // uncomment for UnderLongPress event
                        /* || (indev->tick - cr_longpress->timer > indev->config.longpress_update_interval) */
                        );
                // finger lift
                if (!f->is_active) {
                    CR_RESET(cr_longpress);
                    return;
                }
                /* uncomment for underLongPress event
                cr_longpress->timer = indev->tick;
                cr_longpress->counter ++;
                printf("[EV %d]: Under LongPress, count %d\n", ARRAY_INDEX(f, indev->fingers), cr_longpress->counter);

                CR_YIELD(cr_longpress);
                */
            }

            return;
        }
    }
    CR_END(cr_longpress);
}

// 3.2 Call from detectProcess, Click
// ----------------------------------------
void NotifyClickLong(InputDevice *indev, Finger *f) {
    f->cr_common.is_long = 1;
}

void NotifyClickGesture(InputDevice *indev, Finger *f) {
    f->cr_common.is_gesture = 1;
}


void OnClick(InputDevice *indev, int count, int16_t x, int16_t y);

void ClickReset(InputDevice *indev, Finger *f) {
    cr_click_t * const click = &f->cr_click;
    cr_common_t * const common = &f->cr_common;

    if (click->count != 0) {
        printf("[EV %d]: [%c] Click %d, x %d, y %d\n",
                ARRAY_INDEX(f, indev->fingers),
                (click->count == indev->config.click_max) ? 'M' : 'W',
                click->count,
                click->point.x, click->point.y
              );
        OnClick(index, click->count, click->point.x, click->point.y);
        click->count = 0;
    }

    if (click->is_special) {
        if (common->is_gesture) {
            // do nothing, do not handle event here
        } else if (common->is_long) {
            printf("[EV %d]: LongClick, x %d, y%d\n",
                    ARRAY_INDEX(f, indev->fingers),
                    common->end_point.x, common->end_point.y
                  );
        }
        click->is_special = 0;
    }

}

void Co_Click(InputDevice *indev, Finger *f) {
    cr_click_t * const cr_click = &f->cr_click;
    cr_common_t * const common = &f->cr_common;
    CR_START(cr_click);
    while(1) {
        // cr_click == 0(which is initial state), and on finger lift
        CR_AWAIT(cr_click, !f->is_active && f->is_edge /*&& !indev->curr_slot_mask*/);


        cr_click->is_special = common->is_long || common->is_gesture;
        if (!cr_click->is_special) {
            cr_click->count = 1;
            cr_click->watchdog = indev->tick;
            cr_click->point = f->point;
        }

        // click number reach max, so clean it
        if (cr_click->is_special || cr_click->count == indev->config.click_max) {
            ClickReset(indev, f);
            assert(cr_click->count == 0);
            CR_RESET(cr_click);
            return;
        } else {
            CR_YIELD(cr_click);
        }


        while(1) {
            CR_AWAIT(cr_click,
                    // another last finger lift
                    (!f->is_active && f->is_edge /*&& !indev->curr_slot_mask*/) ||
                    (indev->tick - cr_click->watchdog > indev->config.click_threshold)
                    );
            bool is_not_same = false; // treat as same point


            // if is another click, we should base update cr_click info
            if (!f->is_active && f->is_edge /*&& !indev->curr_slot_mask*/) {
                cr_click->is_special = common->is_long || common->is_gesture;

                if (cr_click->is_special) {
                    // do nothing
                } else if (!(
                            LV_ABS(common->end_point.x - cr_click->point.x) < indev->config.click_region_max &&
                            LV_ABS(common->end_point.y - cr_click->point.y) < indev->config.click_region_max
                            ))
                {
                    is_not_same = true;
                } else {
                    cr_click->count++;
                    cr_click->watchdog = indev->tick;
                    cr_click->point = f->point;
                }


                // not full, rewait next click
                if (!(cr_click->count == indev->config.click_max || cr_click->is_special || is_not_same)) {
                    CR_YIELD(cr_click);
                    continue;
                }
            }

            // if cr_click->count reach clickmax or watachdog bark ClickReset
            ClickReset(indev, f);

            if (is_not_same) {
                cr_click->count = 1;
                cr_click->watchdog = indev->tick;
                cr_click->point = f->point;
                CR_YIELD(cr_click);
                continue;
            } else {
                CR_RESET(cr_click);
                return;
            }
        }
    }
    CR_END(cr_click)
}

// 3.3 Call from detectProcess, Select, similiar to underLongPress, but handle multifinger gracefully
// ----------------------------------------
void NotifySelect(InputDevice *indev, Finger *f) {
    cr_select_t * const select = &indev->cr_select;
    // already taken by other finger
    if(select->current && select->current->is_active) { return; }
    select->current = f;
}

void SelectReset(InputDevice *indev, Finger *f) {
    cr_select_t * const cr_select = &indev->cr_select;
    cr_select->current = NULL;
    cr_select->counter = 0;
    cr_select->timer = 0;
}

void OnSelect(InputDevice *indev, int count, int16_t x, int16_t y);

void Co_Select(InputDevice *indev, Finger *f) {
    cr_select_t * const cr_select = &indev->cr_select;
    CR_START(cr_select);
    while(1) {
        CR_AWAIT(cr_select, cr_select->current);

        cr_select->timer = indev->tick;
        cr_select->counter = 0;

        while(1) {
            CR_AWAIT(cr_select,
                    (!cr_select->current->is_active) ||
                    (indev->tick - cr_select->timer > indev->config.longpress_update_interval)
                    );
            if (!cr_select->current->is_active) {
                printf("[EV %d]: Done Select, count %d\n", ARRAY_INDEX(cr_select->current, indev->fingers), cr_select->counter);
                OnSelect(
                        indev,
                        -1, // special -1 as done select signal
                        cr_select->current->point.x,
                        cr_select->current->point.y
                        );
                SelectReset(indev, f);
                CR_RESET(cr_select);
                return;
            }
            cr_select->timer = indev->tick;
            cr_select->counter ++;

            // printf("[EV %d]: Under Select, count %d\n", ARRAY_INDEX(cr_select->current, indev->fingers), cr_select->counter);
            OnSelect(
                    indev,
                    cr_select->counter,
                    cr_select->current->point.x,
                    cr_select->current->point.y
                    );


            CR_YIELD(cr_select);
        }

    }
    CR_END(cr_select);
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
    // TODO: it's fine for now, since we only have one kind of multigesture,
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
    if (mg->end == indev->config.multigesture_max) {
        MultiGestureReset(indev, f);
    }
}

void Co_MultiGesture(InputDevice *indev, Finger *f) {
    cr_multigesture_t * const cr_mg = &indev->cr_multigesture;
    CR_START(cr_mg);
    while(1) {
        // watchdog bark or any finger which already detect gesture lift
        CR_AWAIT(cr_mg,
                (cr_mg->end != 0 && indev->tick - cr_mg->watchdog > indev->config.multigesture_threshold) ||
                (cr_mg->end != 0 && ((cr_mg->fifo_mask & indev->curr_slot_mask) != cr_mg->fifo_mask))
                )
        MultiGestureReset(indev, f);
        CR_RESET(cr_mg);
        return;
    }
    CR_END(cr_mg)

}

#define VOID_CALLBACK 0
#define SELECT_AND_CONFIRM_CALLBACK 1
#define CALLBACK_TYPE SELECT_AND_CONFIRM_CALLBACK
// ========================================
// Default callback implement
// ========================================
#if defined(CALLBACK_TYPE) && CALLBACK_TYPE == VOID_CALLBACK
void __attribute__((weak)) OnSelect(InputDevice *indev, int count, int16_t x, int16_t y) { }
void __attribute__((weak)) OnClick(InputDevice *indev, int count, int16_t x, int16_t y) { }
void __attribute__((weak)) OnScan(InputDevice *indev) { }
#endif // #if defined(CALLBACK_TYPE) && CALLBACK_TYPE == VOID_CALLBACK

// ========================================
// User example Select and confirm
// ========================================
#if defined(CALLBACK_TYPE) && CALLBACK_TYPE == SELECT_AND_CONFIRM_CALLBACK
typedef enum event_source {
    ONSCAN,
    ONSELECT,
    ONCLICK,
} event_source_t;

typedef struct event {
    event_source_t source;
    union {
        struct {} scan;
        struct { int count; int16_t x; int16_t y; int16_t zoneid } select;
        struct { int count; int16_t x; int16_t y; } click;
    } as;
} event_t;

typedef struct cr_select_confirm {
    CR_FIELD;
    event_t event;

    int16_t zoneid;
    TickType_t timer;

} cr_select_confirm_t;

void Co_Select_Confirm(InputDevice *indev, cr_select_confirm_t* ctx);

cr_select_confirm_t g_ctx = {0};

void OnScan(InputDevice *indev) {
    g_ctx.event.source = ONSCAN;
    // g_ctx.event.as.scan = ?? ;

    Co_Select_Confirm(indev, &g_ctx);
}

int getZoneId(int16_t x, int16_t y) {
    return    (x / (FB_WIDTH / 3))
            + (y / (FB_HEIGHT / 3)) * 3;
}

void OnSelect(InputDevice *indev, int count, int16_t x, int16_t y) {
    g_ctx.event.source = ONSELECT;
    g_ctx.event.as.select = (typeof(g_ctx.event.as.select)){ count, x, y, getZoneId(x, y) };

    Co_Select_Confirm(indev, &g_ctx);
}

void OnClick(InputDevice *indev, int count, int16_t x, int16_t y) {
    g_ctx.event.source = ONCLICK;
    g_ctx.event.as.click = (typeof(g_ctx.event.as.click)){ count, x, y };

    Co_Select_Confirm(indev, &g_ctx);
}



void Co_Select_Confirm(InputDevice *indev, cr_select_confirm_t* ctx) {

    CR_START(ctx);
    while(1) {
        CR_AWAIT(ctx, ctx->event.source == ONSELECT);
        assert(ctx->event.as.select.count == 1);
        ctx->zoneid = ctx->event.as.select.zoneid;
        ctx->timer = indev->tick;
        printf("[EV]: Select code %d\n", ctx->zoneid);

        CR_YIELD(ctx);

        while(1) {
            CR_AWAIT(ctx, ctx->event.source == ONSELECT);

            if (ctx->event.as.select.count == -1) {
                break;
            }

            if (ctx->event.as.select.zoneid == ctx->zoneid) {
                ctx->timer = indev->tick;
            } else if (indev->tick - ctx->timer > DEFAULT_SELECT_AND_CONFIRM_UPDATE_INTERVAL) {
                ctx->timer = indev->tick;
                ctx->zoneid = ctx->event.as.select.zoneid;
                printf("[EV]: Re-Select code %d\n", ctx->zoneid);
            } else {
                // do nothing
            }
            CR_YIELD(ctx);
        }

        ctx->timer = indev->tick;
        CR_YIELD(ctx);

        CR_AWAIT(ctx,
                (ctx->event.source == ONCLICK && ctx->event.as.click.count == 2)
                || ctx->event.source == ONSELECT
                || indev->tick - ctx->timer > DEFAULT_SELECT_AND_CONFIRM_THRESHOLD
                );

        if (ctx->event.source == ONCLICK && ctx->event.as.click.count == 2) {
            printf("[EV]: Confirm code %d\n", ctx->zoneid);
            CR_RESET(ctx);
            return;
        }

        CR_RESET(ctx);
        continue;
    }
    CR_END(ctx);
}
#endif // #if defined(CALLBACK_TYPE) && CALLBACK_TYPE == SELECT_AND_CONFIRM_CALLBACK

// ========================================
// Adapter
// ========================================
void fill_rect(int x, int y, int width, int height, uint32_t color);
void InputDeviceScanCore(InputDevice* indev, ScanData *data) {
    ASSERT_CREATE_MASK(MAX_SUPPORT_SLOT - 1);
    static const uint32_t SUPPORT_MASK = CREATE_MASK(MAX_SUPPORT_SLOT - 1);

    data->slot_mask = MPS2FB_TOUCH->header.points_mask & SUPPORT_MASK;
    // Mock 2/3 finger
    // data->slot_mask |= (data->slot_mask & 1) << 1;
    // data->slot_mask |= (data->slot_mask & 1) << 2;
    fill_rect(0, 0, FB_WIDTH, FB_HEIGHT, 0xFFFFFFFF);

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
        // if (i == 0) {
        //     printf("x: %d, y %d\n", data->touch_points[i].point.x, data->touch_points[i].point.y);
        // }
        if (MPS2FB_TOUCH->points[i].pressed > 0)  {
            #define RECT_SIZE 8
            fill_rect(
                    MPS2FB_TOUCH->points[i].x - RECT_SIZE/2,
                    MPS2FB_TOUCH->points[i].y - RECT_SIZE/2,
                    RECT_SIZE, RECT_SIZE,
                    0xFFFF0000
                    );
        }
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
            printf(
                    "Statistics: HighWaterMark %lu Byte, Rate %d\n",
                    high_water_mark * sizeof(StackType_t),
                    notify_count / (STATISTICS_PERIOD / 1000)
                  );

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
__attribute__((optimize("Ofast")))
void fill_rect(int x, int y, int width, int height, uint32_t color) {
    volatile uint32_t *fb_ptr = FB_BASE_ADDRESS;

    // 使用宏进行边界裁剪
    int start_x = LV_MAX(0, x);
    int start_y = LV_MAX(0, y);
    int end_x = LV_MIN(FB_WIDTH, x + width);
    int end_y = LV_MIN(FB_HEIGHT, y + height);

    if (start_x >= end_x || start_y >= end_y) return;

    int actual_width = end_x - start_x;
    int actual_height = end_y - start_y;

    for (int yi = 0; yi < actual_height; ++yi) {
        uint32_t *row = fb_ptr + (start_y + yi) * FB_WIDTH + start_x;
        for (int xi = 0; xi < actual_width; ++xi) {
            row[xi] = color;
        }
    }
}

void MainTask() {
    // create indev
    DEV = InputDeviceCreate();
    InputDeviceEnable(DEV);

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
