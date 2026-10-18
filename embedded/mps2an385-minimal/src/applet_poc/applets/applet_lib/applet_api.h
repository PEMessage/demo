#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APPLET_EXPORT __attribute__((visibility("default")))

struct applet_api;

typedef struct {
    uint32_t magic;
    uint32_t version;
    const char* name;
    uint32_t (*entry)(const struct applet_api* api, void* arg);
} applet_header_t;

#define APPLET_MAGIC 0x4150504C
#define APPLET_VERSION 1

#define APPLET_EXPORT_HEADER(name_str, entry_func) \
    __attribute__((section(".applet_header"), used)) \
    applet_header_t applet_header = { \
        .magic = APPLET_MAGIC, \
        .version = APPLET_VERSION, \
        .name = name_str, \
        .entry = entry_func \
    }

typedef struct applet_api {
    uint32_t version;
    
    void (*print)(const char* fmt, ...);
    void (*putc)(char c);
    uint32_t (*get_tick)(void);
    void (*delay_ms)(uint32_t ms);
    
    void* (*malloc)(size_t size);
    void (*free)(void* ptr);
    void* (*realloc)(void* ptr, size_t size);
    
} applet_api_t;

#ifdef __cplusplus
}
#endif
