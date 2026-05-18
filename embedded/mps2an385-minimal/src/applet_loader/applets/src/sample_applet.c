#include "applet_api.h"
#include <stddef.h>

// Declare external functions provided by the loader
int printf(const char* fmt, ...);
void* malloc(size_t size);
void free(void* ptr);

char global_string[] = "Hello World from global";
static char static_global_string[] = "Hello World from static global";


char empty_arrow[128] = {0};

static uint32_t applet_main(const applet_api_t* api, void* arg) {
    (void)api;
    (void)arg;

    printf("&global_string is %p\n", global_string);
    printf("global_string is %s\n", global_string);
    printf("&static_global_string is %p\n", static_global_string);
    printf("static_global_string is %s\n", static_global_string);
    printf("Hello from sample applet!\n");

    printf("Tick: %lu\n", api->get_tick());

    printf("empty_arrow:\n  ");
    for (int i = 0 ; i < sizeof(empty_arrow) ; i ++) {
        printf("%02X ", empty_arrow[i]);
    }
    printf("\n");


    void* p = malloc(64);
    printf("malloc(64) = %p\n", p);
    free(p);

    for (int i = 0; i < 5; i++) {
        printf("Count: %d\n", i);
        api->delay_ms(500);
    }

    printf("Sample applet finished!\n");
    return 42;
}

APPLET_EXPORT_HEADER("sample_applet", applet_main);
