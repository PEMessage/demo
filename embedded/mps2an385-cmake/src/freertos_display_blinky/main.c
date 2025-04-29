#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "task.h"


#include <stdint.h>


// Define framebuffer properties based on the QEMU code
#define FB_BASE_ADDRESS ((volatile uint8_t*)0x41000000)
#define FB_WIDTH        1120
#define FB_HEIGHT       832
#define FB_BPP          2 // Bits per pixel
#define FB_PIXELS_PER_BYTE (8 / FB_BPP) // Should be 4
#define FB_BYTES_PER_ROW (FB_WIDTH / FB_PIXELS_PER_BYTE) // 1120 / 4 = 280

// Define color indices (matching the deduction from the QEMU palette)
#define COLOR_IDX_WHITE     0 // 00
#define COLOR_IDX_LT_GRAY   1 // 01
#define COLOR_IDX_DK_GRAY   2 // 10
#define COLOR_IDX_BLACK     3 // 11

/**
 * @brief Fills the NeXT framebuffer with a single color.
 *
 * @param color_index The 2-bit color index (0-3) to fill the screen with.
 * Use COLOR_IDX_WHITE, COLOR_IDX_LT_GRAY, etc.
 */
void fill_framebuffer(uint8_t color_index) {
    // Ensure the color index is valid (only 2 bits)
    color_index &= 0x03;

    // Create a byte where all 4 pixels have the desired color index.
    // Pixel order in byte: [P0 P1 P2 P3] -> Bits: [76 54 32 10]
    // So we need: (color_index << 6) | (color_index << 4) | (color_index << 2) | color_index
    uint8_t fill_byte = (color_index << 6) | (color_index << 4) | (color_index << 2) | color_index;

    volatile uint8_t *fb_ptr = FB_BASE_ADDRESS;
    uint32_t y, x_byte; // Use x_byte for iterating through bytes horizontally

    // Iterate through each row
    for (y = 0; y < FB_HEIGHT; ++y) {
        // Iterate through the bytes making up the row
        for (x_byte = 0; x_byte < FB_BYTES_PER_ROW; ++x_byte) {
            // Write the pre-calculated byte containing 4 pixels
            *fb_ptr = fill_byte;
            fb_ptr++; // Move to the next byte in memory
        }
        // Optional: If there's padding at the end of a row (stride != width),
        // add pointer adjustment here. Based on the QEMU code analysis,
        // it seems tightly packed (FB_BYTES_PER_ROW = 280), so no padding needed.
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
    while(1) {
        printf("Ping!\n");
        vTaskDelay(1000);
        fill_framebuffer(color);
        color = ( color + 1 ) % 4;
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
