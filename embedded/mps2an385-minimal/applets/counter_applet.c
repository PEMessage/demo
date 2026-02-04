#include "applet_api.h"

static uint32_t counter_applet_main(const applet_api_t* api, void* arg) {
    api->print("\n=== Counter Applet ===\n");
    
    for (int i = 10; i >= 0; i--) {
        api->print("Countdown: %d\n", i);
        api->delay_ms(300);
    }
    
    api->print("Blast off!\n");
    return 100;
}

APPLET_EXPORT_HEADER("counter_applet", counter_applet_main);
