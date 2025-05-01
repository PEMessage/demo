#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"


#include <stdint.h>


// Define framebuffer properties based on the QEMU code
#define FB_BASE_ADDRESS ((volatile uint32_t*)0x41000000)
#define FB_WIDTH        640
#define FB_HEIGHT       480
#define FB_BPP          32 // Bits per pixel
#define FB_BYTE_PER_PIXEL (FB_BPP / 8 ) // Should be 4
#define FB_BYTES_PER_ROW (FB_WIDTH * FB_BYTE_PER_PIXEL) // 1120 / 4 = 280

void NVIC_Init() {
    size_t i = 0 ;
    for ( i = 0; i < 32; i++) {
        // NVIC_DisableIRQ(i);
        NVIC_SetPriority((IRQn_Type)i, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    }
}

void setup() {
    extern int stdout_init (void);
    stdout_init();
    NVIC_Init();
}


// ======================================== 
// Helper function
// ======================================== 


#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
static TimerHandle_t lvgl_tick_timer;

void lvgl_tick_timer_cb(TimerHandle_t xTimer)
{
    lv_tick_inc(1); // Notify LVGL that 1ms has passed
}

void init_lvgl_tick(void)
{
    // Create a one-shot timer that auto-reloads every 1 ms
    lvgl_tick_timer = xTimerCreate("LVGL Tick", pdMS_TO_TICKS(1), pdTRUE, NULL, lvgl_tick_timer_cb);
    if (lvgl_tick_timer != NULL) {
        xTimerStart(lvgl_tick_timer, 0);
        printf("Start LVGL Timer!\n");
    }
}


SemaphoreHandle_t lvgl_init_semaphore;
SemaphoreHandle_t app_init_semaphore;

void TaskLVGL() {
    /*Change the active screen's background color*/
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    init_lvgl_tick();
    printf("LVGL Init complete\n");
    xSemaphoreGive(lvgl_init_semaphore);

    if (xSemaphoreTake(app_init_semaphore, portMAX_DELAY) == pdTRUE) {
        printf("Application UI setup complete. Starting LVGL loop.\n");
    }

    while(1) {
        lv_task_handler();
    }

}

static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
        static uint8_t cnt = 0;
        cnt++;

        /*Get the first child of the button which is the label and change its text*/
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Button: %d", cnt);
    }
}

void lv_example_get_started_2(void)
{
    lv_obj_t * btn = lv_button_create(lv_screen_active());     /*Add a button the current screen*/
    lv_obj_set_pos(btn, 10, 10);                            /*Set its position*/
    lv_obj_set_size(btn, 120, 50);                          /*Set its size*/
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);           /*Assign a callback to the button*/

    lv_obj_t * label = lv_label_create(btn);          /*Add a label to the button*/
    lv_label_set_text(label, "Button");                     /*Set the labels text*/
    lv_obj_center(label);
}


void TaskAppUI(void *pvParameters) {
    /* Wait for LVGL initialization to complete */
    if (xSemaphoreTake(lvgl_init_semaphore, portMAX_DELAY)) {
        printf("Starting UI setup\n");
    }


    lv_example_get_started_2();

    xSemaphoreGive(app_init_semaphore);

    /* Rainbow color animation loop */
    uint16_t hue = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // ~20 FPS
    }

}


int main() {
    setup();
    printf("Init complete\n");
    lvgl_init_semaphore = xSemaphoreCreateBinary();
    app_init_semaphore  = xSemaphoreCreateBinary();
    xTaskCreate( TaskLVGL,
            "TaskLVGL",
            configMINIMAL_STACK_SIZE + 3*1024,
            NULL,
            (tskIDLE_PRIORITY + 1),
            NULL );
        /* Create App UI Task */
    xTaskCreate(TaskAppUI,
                "TaskAppUI",
                configMINIMAL_STACK_SIZE + 2 * 1024,
                NULL,
                (tskIDLE_PRIORITY + 1),
                NULL);
    vTaskStartScheduler();
    while(1);
}
