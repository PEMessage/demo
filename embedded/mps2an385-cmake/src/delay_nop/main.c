#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock
#include "cmsis_gcc.h"



void NVIC_Init() {
    size_t i = 0 ;
    for ( i = 0; i < 32; i++) {
        NVIC_SetPriority((IRQn_Type)i, 5);
    }
}

void SysTick_Init() {
    SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk;
    NVIC_DisableIRQ(SysTick_IRQn);
}

void setup() {
    extern int stdout_init (void);
    stdout_init();
    NVIC_Init();
    SysTick_Init();

    printf("Starting NOP calibration...\n");
    printf("SystemCoreClock is %u\n", SystemCoreClock);
}


volatile uint32_t loop_time = 112100;
void delay_1us() {
    uint32_t loop = loop_time;
    do {
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
    } while(loop--);
}

void measure_loop_times() {
    uint32_t tick_load =  (SystemCoreClock / 1000000) - 1; // Expected VAL after 1µs

    printf("Testing tick_load=%u)\n", tick_load);

    while(1) {
        // 1. Measurement 
        // ------------------------------
        uint32_t cnt = 0;
        uint32_t ctrl = 0;
        // Disable SysTick and reset
        SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
        SysTick->LOAD  = tick_load;
        SysTick->VAL = 0;

        // Enable SysTick
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

        // Execute the delay
        delay_1us();
        do
        {
            cnt++;
            ctrl = SysTick->CTRL;
        } while( (ctrl & SysTick_CTRL_ENABLE_Msk) && ! (ctrl & SysTick_CTRL_COUNTFLAG_Msk));		//等待时间到达

        // Disable SysTick and read value
        SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
        // SysTick->VAL = 0;

        // 2. Tweak 
        // ------------------------------
        printf("Testing loop_time=%u, cnt=%u (tick_load=%u)\n",
                loop_time, cnt, tick_load);

        if (cnt > 1) {
            // Delay was too short - increase NOP count
            loop_time++;
        }
        if (cnt <= 1 ) {
            // Delay was too long - decrease NOP count
            if (loop_time > 1) loop_time--;
        }
    }
}


int main() {
    setup();
    measure_loop_times();
    while(1);
}



