#include <stdio.h>
#include "CMSDK_CM3.h" // for SystemCoreClock


void NVIC_Init() {
    size_t i = 0 ;
    for ( i = 0; i < 32; i++) {
        NVIC_DisableIRQ(i);
        NVIC_SetPriority((IRQn_Type)i, 5);
    }
}
void SysTick_Init() {
    SysTick_Config(SystemCoreClock / 4); // 4Hz
    NVIC_EnableIRQ(SysTick_IRQn);
}

void setup() {
    extern int stdout_init (void);
    stdout_init();
    NVIC_Init();
    SysTick_Init();
}

void SysTick_Handler() {
    printf("Hello world, Tick, and system clock is: %d\n", SystemCoreClock);
}


int main() {
    setup();
    while(1);
}
