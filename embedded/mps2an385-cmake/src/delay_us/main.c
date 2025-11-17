#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "CMSDK_CM3.h" // for SystemCoreClock

// Global variables for delay functionality
static volatile uint64_t systick_count = 0;
static uint64_t systick_reload_value = 0;

void NVIC_Init() {
    int i = 0;
    for (i = 0; i < 32; i++) {
        NVIC_DisableIRQ(i);
        NVIC_SetPriority((IRQn_Type)i, 5);
    }
}

void SysTick_Init() {
    systick_reload_value = SystemCoreClock / (1000 * 1000); // 1MHz = 1us per tick

    // NOTE: it interupt way too fast, so qemu is not that fast, you might get 100us per tick

    printf("systick_reload_value is %"PRIu64"\n", systick_reload_value);
    SysTick_Config(systick_reload_value);
    NVIC_EnableIRQ(SysTick_IRQn);
}

void SysTick_Handler() {
    systick_count++;
}

void delay_us(uint64_t us) {
    uint64_t start_time = systick_count;
    while ((systick_count - start_time) < us) {}
}

void delay_ms(uint64_t ms) {
    if(!ms) {return;}
    while(ms--) {delay_us(10);} // it should be 1000
    // since qemu not that fast, adjust it to 10 instead 1000
}


void setup() {
    extern int stdout_init (void);
    stdout_init();
    NVIC_Init();
    SysTick_Init();
}

int main() {
    setup();

    while(1) {
        printf("Hello world, Tick, and system clock is: %d\n", SystemCoreClock);
        delay_ms(1000);  // Delay for 1 second
    }
}
