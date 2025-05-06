#include <src/core/lv_obj_style.h>
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

static void anim_x_cb(void * var, int32_t v)
{
    lv_obj_t * obj = (lv_obj_t *)var;
    lv_obj_set_x(obj, v);
}

static void anim_size_cb(void * var, int32_t v)
{
    lv_obj_t * obj = (lv_obj_t *)var;
    lv_obj_set_size(obj, v, v);
    printf("v is %d\n",v);
}

static void btn_event_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    
    // Create X-axis bounce animation
    lv_anim_t anim_x;
    lv_anim_init(&anim_x);
    lv_anim_set_var(&anim_x, btn);
    lv_anim_set_values(&anim_x, lv_obj_get_x(btn), lv_obj_get_x(btn) + 50);
    lv_anim_set_time(&anim_x, 1000);
    lv_anim_set_exec_cb(&anim_x, anim_x_cb);
    lv_anim_set_path_cb(&anim_x, lv_anim_path_bounce);
    lv_anim_set_playback_time(&anim_x, 500);
    lv_anim_set_repeat_count(&anim_x, 1);
    lv_anim_start(&anim_x);

    // Create size bounce animation
    lv_anim_t anim_size;
    lv_anim_init(&anim_size);
    lv_anim_set_var(&anim_size, btn);
    lv_anim_set_values(&anim_size, lv_obj_get_height(btn), lv_obj_get_height(btn) + 100);
    lv_anim_set_time(&anim_size, 1000);
    lv_anim_set_exec_cb(&anim_size, anim_size_cb);
    lv_anim_set_path_cb(&anim_size, lv_anim_path_bounce);
    lv_anim_set_playback_time(&anim_size, 500);
    lv_anim_set_repeat_count(&anim_size, 1);
    lv_anim_start(&anim_size);
}


void TaskAppUI(void *pvParameters) {
    /* Wait for LVGL initialization to complete */
    if (xSemaphoreTake(lvgl_init_semaphore, portMAX_DELAY)) {
        printf("Starting UI setup\n");
    }


    lv_obj_t * btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 100, 100);

    // NOTE:=====================================
    //      lv_obj_center(btn);  
    // NOTE: algin center will casuse button counldn't brounce back
    // NOTE: it might relate to algin style center, so we need put it manully 
    // NOTE:=====================================

    lv_coord_t screen_width = lv_obj_get_width(lv_scr_act());
    lv_coord_t screen_height = lv_obj_get_height(lv_scr_act());
    lv_obj_set_pos(btn, (screen_width - 100) / 2, (screen_height - 100) / 2);

    lv_obj_remove_style(btn,  NULL,LV_STYLE_ALIGN);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

    // Add a label to the button
    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, "Click me!");
    lv_obj_center(label);

    // Style the button
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_radius(btn, 20, 0);

    xSemaphoreGive(app_init_semaphore);

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
