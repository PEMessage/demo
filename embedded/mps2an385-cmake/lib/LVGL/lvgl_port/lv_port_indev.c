/**
 * @file lv_port_indev.c
 *
 */

/*Copy this file as "lv_port_indev.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"
#include <stdint.h>

/*********************
 *      DEFINES
 *********************/
#include "CMSDK_CM3_EXT.h"


#define TIMER_MODE 0
#define EVENT_MODE 1

#ifndef LV_PORT_INDEV_MODE
    #define LV_PORT_INDEV_MODE TIMER_MODE
    #define LV_INDEV_MODE  LV_INDEV_MODE_TIMER

#else
    #define LV_PORT_INDEV_MODE EVENT_MODE
    #define LV_INDEV_MODE  LV_INDEV_MODE_EVENT

    #include "FreeRTOS.h"

    static void touch_pend_callback(void *dev, uint32_t event); // Added for event mode
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void touchpad_init(void);
static void touchpad_read(lv_indev_t * indev, lv_indev_data_t * data);

/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t * indev_touchpad;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    /*------------------
     * Touchpad
     * -----------------*/

    /*Initialize your touchpad*/
    touchpad_init();

    /*Register a touchpad input device*/
    indev_touchpad = lv_indev_create();
    lv_indev_set_mode(indev_touchpad, LV_INDEV_MODE);

    lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad, touchpad_read);

    /*We don't need other input devices for this example*/
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*------------------
 * Touchpad
 * -----------------*/

/*Initialize your touchpad*/
static void touchpad_init(void) {
#if LV_PORT_INDEV_MODE == EVENT_MODE
    TOUCH_CTRL->enable_irq = 1;
    TOUCH_CTRL->reserved = 77; // random number to verify struct work
    NVIC_EnableIRQ(Touch_IRQn);
#endif
}

/*Will be called by the library to read the touchpad*/
static void touchpad_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
    #if defined(LV_PORT_INDEV_SUPPORT_MULTI_TOUCH) && LV_PORT_INDEV_SUPPORT_MULTI_TOUCH == 1
    lv_indev_touch_data_t touches[MAX_TOUCH_POINTS];

    int touch_cnt = MAX_TOUCH_POINTS ;

    for (int i = 0; i < MAX_TOUCH_POINTS ; i++) {
        touches[i].point.x = MPS2FB_TOUCH->points[i].x;
        touches[i].point.y = MPS2FB_TOUCH->points[i].y;
        touches[i].state = MPS2FB_TOUCH->points[i].pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
        touches[i].id = i;
    }

    lv_indev_gesture_recognizers_update(indev_drv, touches, touch_cnt);
    lv_indev_gesture_recognizers_set_data(indev_drv, data);
    #endif

    // NOTE: this is also important for mutlitouch
    // without trdition single pointer data. all multitouch will not work.
    // and should be one of touches

    data->point.x = MPS2FB_TOUCH->points[0].x;
    data->point.y = MPS2FB_TOUCH->points[0].y;
    data->state =  MPS2FB_TOUCH->points[0].pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;

}

/*------------------
 * Touchpad Event Mode Extra
 * -----------------*/
#if LV_PORT_INDEV_MODE == EVENT_MODE
/* Event mode callback function - called from ISR via FreeRTOS */

void touch_pend_callback(void * arg1, uint32_t arg2) {
    struct lv_indev_t *indev = (lv_indev_t *)arg1;
    lv_indev_read(indev);
}
/* Touch Interrupt Handler */
void TouchIRQ_Handler(void) {
    static uint32_t prev_pressed = 0;
    uint32_t prev = prev_pressed;
    uint32_t curr = *TOUCH_PRESS;
    prev_pressed = curr; // update prev

    #define NO_HOVER_SUPPORT
    #ifdef NO_HOVER_SUPPORT
    // if neither edge nor pressed skip it -- skip hover event
    if (!( (prev ^ curr) || curr )) {
        return;
    }
    #endif

    // Defer the touch processing to the RTOS task context
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTimerPendFunctionCallFromISR(
            touch_pend_callback,
            indev_touchpad,  // No specific device context needed
            0,     // No event parameter needed
            &xHigherPriorityTaskWoken
            );

    // Perform context switch if necessary
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

#endif

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
