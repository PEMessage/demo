#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "CMSDK_CM3_EXT.h" // for SystemCoreClock

void NVIC_Init() {
    size_t i = 0 ;
    for ( i = 0; i < 32; i++) {
        // NVIC_DisableIRQ(i);
        NVIC_SetPriority((IRQn_Type)i,  4);
    }
}

void SysTick_Init() {
    SysTick_Config(SystemCoreClock / 4); // 4Hz
    NVIC_EnableIRQ(SysTick_IRQn);
}

void enable_touch_irq() {
    TOUCH_CTRL->enable_irq = 1;
    TOUCH_CTRL->reserved = 77; // random number to verify struct work
    NVIC_EnableIRQ(Touch_IRQn);
}

void TouchIRQ_Handler() {
    printf("Hello TouchIRQ, pend: %d, x: %4d, y: %4d, pressed %d\n",
            NVIC_GetPendingIRQ(Touch_IRQn),
            *TOUCH_X,
            *TOUCH_Y,
            *TOUCH_PRESS
          );
}

void SysTick_Handler() {
    printf("Hello world, Tick, and system clock is: %d\n", SystemCoreClock);
}


void setup() {
    extern int stdout_init (void);
    stdout_init();
    NVIC_Init();
    SysTick_Init();
    enable_touch_irq();
}

int main() {
    setup();
    printf("Init complete\n");
    while(1);
}
