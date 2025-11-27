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


typedef struct InputDevice {
    bool enabled : 1;

    uint32_t prev_slot_mask;
    uint32_t curr_slot_mask;

    TickType_t tick;
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
                // data.touch_points[i] = indev->fingers[i].touch_point;
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
                // indev->fingers[i].touch_point = data.touch_points[i];
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
#define STATISTICS_PERIOD 5
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
        if ((xTaskGetTickCount() - xLastWakeTime) > pdMS_TO_TICKS(STATISTICS_PERIOD * 1000)) {
            // Get high water mark
            UBaseType_t high_water_mark = uxTaskGetStackHighWaterMark(NULL);

            // Print statistics
            printf("Statistics: HighWaterMark %lu, Rate %d\n", high_water_mark, notify_count / STATISTICS_PERIOD);

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
const uint32_t PERIOD = 50; // ms
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
    int color_counter = 0;

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
            configMINIMAL_STACK_SIZE + 1024,  // Adjust stack size as needed
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
