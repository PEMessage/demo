#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "CMSDK_CM3_EXT.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "task.h"


#include <stdint.h>


// Define framebuffer properties based on the QEMU code
// #define FB_BASE_ADDRESS ((volatile uint32_t*)(0x41001000))
// #define FB_WIDTH        640
// #define FB_HEIGHT       480
// #define FB_BPP          32 // Bits per pixel
// #define FB_BYTE_PER_PIXEL (FB_BPP / 8 ) // Should be 4
// #define FB_BYTES_PER_ROW (FB_WIDTH * FB_BYTE_PER_PIXEL) // 1120 / 4 = 280

// Define color indices (matching the deduction from the QEMU palette)
#define COLOR_IDX_BLACK     0 
#define COLOR_IDX_DK_GRAY   1 
#define COLOR_IDX_LT_GRAY   2 
#define COLOR_IDX_WHITE     3 
#define COLOR_IDX_RED       4
#define COLOR_IDX_GREEN     5
#define COLOR_IDX_BLUE      6
#define COLOR_IDX_NUM       7 

/**
 * @brief Fills the NeXT framebuffer with a single color.
 *
 * @param color_index The 2-bit color index (0-3) to fill the screen with.
 * Use COLOR_IDX_WHITE, COLOR_IDX_LT_GRAY, etc.
 */
void fill_framebuffer(uint8_t color_index) {
    static uint32_t map_of_color[] = {
        0x000000,
        0x555555,
        0xaaaaaa,
        0xffffff,
        0xff0000,
        0x00ff00,
        0x0000ff,
    };
    uint32_t fill_byte = map_of_color[color_index % COLOR_IDX_NUM];

    volatile uint32_t *fb_ptr = FB_BASE_ADDRESS;
    uint32_t y, x; // Use x_byte for iterating through bytes horizontally
    int color_counter = 0;

    for (y = 0; y < FB_HEIGHT; ++y) {
        
        for (x = 0; x < FB_WIDTH; ++x) {
            *fb_ptr = fill_byte;
            fb_ptr++; // Move to the next byte in memory
        }
        color_counter = ( color_counter + 1 ) % 10;
        if ( color_counter == 0 ) {
            fill_byte = map_of_color[++color_index  % COLOR_IDX_NUM];
        }
    }
}


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

void TaskSwitchColor() {
    static int color = 0 ;
    static TickType_t previousTimestamp = 0;
    while(1) {
        TickType_t currentTimestamp = xTaskGetTickCount();
        TickType_t timeDiff = currentTimestamp - previousTimestamp;
        printf("Time since last run: %u ticks\n", timeDiff);
        previousTimestamp = currentTimestamp;

        vTaskDelay(128);
        fill_framebuffer(color);
        color = ( color + 1 ) % COLOR_IDX_NUM;
    }

}


int main() {
    setup();
    printf("Init complete\n");
    xTaskCreate( TaskSwitchColor,
            "TaskSwitchColor",
            configMINIMAL_STACK_SIZE + 3*1024,
            NULL,
            (tskIDLE_PRIORITY + 1),
            NULL );
    vTaskStartScheduler();
    while(1);
}
