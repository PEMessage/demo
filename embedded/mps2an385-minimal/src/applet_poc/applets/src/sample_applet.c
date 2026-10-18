#include "applet_api.h"

static const applet_api_t* g_api = NULL;

static uint32_t applet_main(const applet_api_t* api, void* arg) {
    g_api = api;
    
    api->print("Hello from sample applet!\n");
    api->print("Tick: %lu\n", api->get_tick());
    
    for (int i = 0; i < 5; i++) {
        api->print("Count: %d\n", i);
        api->delay_ms(500);
    }
    
    api->print("Sample applet finished!\n");
    return 42;
}

APPLET_EXPORT_HEADER("sample_applet", applet_main);
