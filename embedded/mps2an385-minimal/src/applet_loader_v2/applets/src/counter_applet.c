#include "applet_api.h"

// Declare external function provided by the loader
int printf(const char* fmt, ...);

static uint32_t counter_applet_main(const applet_api_t* api, void* arg) {
    (void)arg;

    printf("\n=== Counter Applet ===\n");
    
    for (int i = 10; i >= 0; i--) {
        printf("Countdown: %d\n", i);
        api->delay_ms(300);
    }
    
    printf("Blast off!\n");
    return 100;
}

APPLET_EXPORT_HEADER("counter_applet", counter_applet_main);
