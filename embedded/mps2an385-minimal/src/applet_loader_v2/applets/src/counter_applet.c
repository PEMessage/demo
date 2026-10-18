#include "applet_api.h"

// Declare external function provided by the loader
int printf(const char* fmt, ...);

// Imported from sample_applet (loaded before us). Resolved at load time by the
// loader's symbol registry — not hard-coded in the host.
extern uint32_t sample_applet_add(uint32_t a, uint32_t b);

static uint32_t counter_applet_main(const applet_api_t* api, void* arg) {
    (void)arg;

    printf("\n=== Counter Applet ===\n");

    printf("Calling sample_applet_add(20, 22) = %lu\n",
           (unsigned long)sample_applet_add(20, 22));

    for (int i = 10; i >= 0; i--) {
        printf("Countdown: %d\n", i);
        api->delay_ms(300);
    }
    
    printf("Blast off!\n");
    return 100;
}

APPLET_EXPORT_HEADER("counter_applet", counter_applet_main);
