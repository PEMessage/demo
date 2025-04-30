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
        printf("Start lvgl!\n");
    }
}

void TaskSwitchColor() {
    /*Change the active screen's background color*/
    lv_init();
    lv_port_disp_init();
    init_lvgl_tick();


    printf("Init complete3\n");
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x003a57), LV_PART_MAIN);

    /*Create a white label, set its text and align it to the center*/
    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello world");
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    while(1) {
        vTaskDelay(30);
        printf("Pong!\n");
        lv_task_handler();
    }

}
int main() {
    setup();
    printf("Init complete1\n");
    printf("Init complete2\n");
    xTaskCreate( TaskSwitchColor,
            "TaskSwitchColor",
            configMINIMAL_STACK_SIZE + 3*1024,
            NULL,
            (tskIDLE_PRIORITY + 1),
            NULL );
    vTaskStartScheduler();
    while(1);
}
