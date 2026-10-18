#ifndef APPLET_API_H
#define APPLET_API_H

#include <stdint.h>
#include <stddef.h>

#define APPLET_EXPORT __attribute__((visibility("default")))

struct applet_api;

typedef struct {
    uint32_t magic;
    uint32_t version;
    const char* name;
    uint32_t (*entry)(const struct applet_api* api, void* arg);
    void * bss_start;
    void * bss_end;
    uint32_t tail;
} applet_header_t;

#define APPLET_MAGIC 0x4150504C
#define APPLET_VERSION 3

// keep this value same as applets/applet_lib/applet.lds
#define APPLET_LINK_ADDR 0x40000000


// NOTICE:
// __attribute__ line must below extern declare, otherwise not work
#define APPLET_EXPORT_HEADER(name_str, entry_func) \
    extern char __bss_start[]; \
    extern char __bss_end[]; \
    __attribute__((section(".applet_header"), used)) \
    applet_header_t applet_header = { \
        .magic = APPLET_MAGIC, \
        .version = APPLET_VERSION, \
        .name = name_str, \
        .entry = entry_func, \
        .bss_start = __bss_start, \
        .bss_end = __bss_end, \
        .tail = APPLET_MAGIC, \
    }

typedef struct applet_api {
    uint32_t version;
    
    int (*print)(const char* fmt, ...);
    void (*putc)(char c);
    uint32_t (*get_tick)(void);
    void (*delay_ms)(uint32_t ms);
    
    void* (*malloc)(size_t size);
    void (*free)(void* ptr);
    void* (*realloc)(void* ptr, size_t size);
    
} applet_api_t;

#endif /* end of include guard: APPLET_API_H */
