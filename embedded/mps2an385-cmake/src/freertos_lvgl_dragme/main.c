#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "CMSDK_CM3_EXT.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include <stdint.h>

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

// ========================================
// Drag Example Implementation
// ========================================

static void drag_event_handler(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);

    lv_indev_t * indev = lv_indev_active();
    if(indev == NULL)  return;

    lv_point_t vect;
    lv_indev_get_vect(indev, &vect);

    int32_t x = lv_obj_get_x(obj) + vect.x;
    int32_t y = lv_obj_get_y(obj) + vect.y;
    lv_obj_set_pos(obj, x, y);
}

/**
 * Make an object draggable.
 */
void lv_example_drag(void)
{
    // Create the main draggable object
    lv_obj_t * obj;
    obj = lv_obj_create(lv_screen_active());
    lv_obj_set_size(obj, 150, 100);
    lv_obj_set_pos(obj, 50, 50); // Initial position
    lv_obj_add_event_cb(obj, drag_event_handler, LV_EVENT_PRESSING, NULL);

    // Add label to the object
    lv_obj_t * label = lv_label_create(obj);
    lv_label_set_text(label, "Drag me");
    lv_obj_center(label);
}

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
        vTaskDelay(pdMS_TO_TICKS(5)); // Small delay to prevent hogging CPU
    }
}

void TaskAppUI(void *pvParameters) {
    /* Wait for LVGL initialization to complete */
    if (xSemaphoreTake(lvgl_init_semaphore, portMAX_DELAY)) {
        printf("Starting UI setup\n");
    }

    // Create the drag example
    lv_example_drag();

    xSemaphoreGive(app_init_semaphore);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Just idle
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
