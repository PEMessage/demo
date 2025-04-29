#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "FreeRTOS.h"
#include "task.h"


#include <stdint.h>


// Define framebuffer properties based on the QEMU code
#define FB_BASE_ADDRESS ((volatile uint32_t*)0x41000000)
#define FB_WIDTH        640
#define FB_HEIGHT       480
#define FB_BPP          32 // Bits per pixel
#define FB_BYTE_PER_PIXEL (FB_BPP / 8 ) // Should be 4
#define FB_BYTES_PER_ROW (FB_WIDTH * FB_BYTE_PER_PIXEL) // 1120 / 4 = 280

typedef uint32_t (*DrawFunc)(int x, int y, uint16_t time);




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

void fill_framebuffer(uint16_t time, DrawFunc func) {

    volatile uint32_t *fb_ptr = FB_BASE_ADDRESS;
    uint32_t y, x; // Use x_byte for iterating through bytes horizontally

    for (y = 0; y < FB_HEIGHT; ++y) {
        
        for (x = 0; x < FB_WIDTH; ++x) {
            *fb_ptr = func(x, y, time);
            fb_ptr++; // Move to the next byte in memory
        }
    }
}

uint32_t draw(int x, int y, uint16_t time);
void TaskSwitchColor() {
    static int time = 0 ;
    static TickType_t previousTimestamp = 0;
    while(1) {
        TickType_t currentTimestamp = xTaskGetTickCount();
        TickType_t timeDiff = currentTimestamp - previousTimestamp;
        printf("Time since last run: %u ticks\n", timeDiff);
        previousTimestamp = currentTimestamp;

        vTaskDelay(128);
        fill_framebuffer(time, draw);
        time = ( time + 1 ) % 60;
    }

}


unsigned char RD(int i,int j);
unsigned char GR(int i,int j);
unsigned char BL(int i,int j);
uint32_t draw(int x, int y, uint16_t time) {
    (void)time;

    uint32_t ret = ( \
            ( RD(x, y) << (2 * 8) ) +
            ( GR(x, y) << (1 * 8) ) +
            ( BL(x, y) << (0 * 8) ) 
            );
    return ret;
}

// ======================================== 
// Real render function 
// ======================================== 
// Thanks to: 有没有一段代码，让你为人类的智慧击节叫好？ - 烧茄子的回答 - 知乎
// https://www.zhihu.com/question/30262900/answer/48741026
// https://codegolf.stackexchange.com/questions/35569/tweetable-mathematical-art
#include <math.h>

#define _sq(x) ((x)*(x)) // square

unsigned char RD(int x, int y) {
    double angle = atan2(y - FB_HEIGHT/2, x - FB_WIDTH/2);
    double half_angle = angle / 2;
    double phase_adjusted = half_angle;
    double cosine = cos(phase_adjusted);
    double squared = _sq(cosine);
    double scaled = squared * 255;

    return (unsigned char)scaled;
}

unsigned char GR(int x, int y) {
    double angle = atan2(y - FB_HEIGHT/2, x - FB_WIDTH/2);
    double half_angle = angle / 2;
    double phase_adjusted = half_angle - 2 * acos(-1) / 3;
    double cosine = cos(phase_adjusted);
    double squared = _sq(cosine);
    double scaled = squared * 255;

    return (unsigned char)scaled;
}

unsigned char BL(int x, int y) {
    double angle = atan2(y - FB_HEIGHT/2, x - FB_WIDTH/2);
    double half_angle = angle / 2;
    double phase_adjusted = half_angle + 2 * acos(-1) / 3;
    double cosine = cos(phase_adjusted);
    double squared = _sq(cosine);
    double scaled = squared * 255;

    return (unsigned char)scaled;
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
