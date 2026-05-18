#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include "applet_api.h"

extern int stdout_init(void);

// ========== dynamic section struct ==========
typedef struct {
    int32_t d_tag;
    union {
        uint32_t d_val;
        uint32_t d_ptr;
    } d_un;
} Elf32_Dyn;

#define ELF_DYNAMIC_TAGS                                                      \
    X(DT_NULL,       0)                                                       \
    X(DT_NEEDED,     1)                                                       \
    X(DT_PLTRELSZ,   2)                                                       \
    X(DT_PLTGOT,     3)                                                       \
    X(DT_HASH,       4)                                                       \
    X(DT_STRTAB,     5)                                                       \
    X(DT_SYMTAB,     6)                                                       \
    X(DT_RELA,       7)                                                       \
    X(DT_RELASZ,     8)                                                       \
    X(DT_RELAENT,    9)                                                       \
    X(DT_STRSZ,     10)                                                       \
    X(DT_SYMENT,    11)                                                       \
    X(DT_INIT,      12)                                                       \
    X(DT_FINI,      13)                                                       \
    X(DT_SONAME,    14)                                                       \
    X(DT_RPATH,     15)                                                       \
    X(DT_SYMBOLIC,  16)                                                       \
    X(DT_REL,       17)                                                       \
    X(DT_RELSZ,     18)                                                       \
    X(DT_RELENT,    19)                                                       \
    X(DT_PLTREL,    20)                                                       \
    X(DT_DEBUG,     21)                                                       \
    X(DT_TEXTREL,   22)                                                       \
    X(DT_JMPREL,    23)                                                       \
    X(DT_BIND_NOW,  24)                                                       \
    X(DT_INIT_ARRAY,      25)                                                 \
    X(DT_FINI_ARRAY,      26)                                                 \
    X(DT_INIT_ARRAYSZ,    27)                                                 \
    X(DT_FINI_ARRAYSZ,    28)                                                 \
    X(DT_RUNPATH,         29)                                                 \
    X(DT_FLAGS,           30)

typedef enum dynamic_tag {
#define X(name, value) name = value,
    ELF_DYNAMIC_TAGS
#undef X
    DT_NUM_TAGS              /* optional sentinel, not a real tag */
} dynamic_tag_t;

const char* dynamic_tag_string(dynamic_tag_t tag) {
    switch (tag) {
#define X(name, value) case name: return #name;
        ELF_DYNAMIC_TAGS
#undef X
        default: return "DT_UNKNOWN";
    }
}


// ========== ELF structures for dynamic linking ==========
// ARM relocation types
#define R_ARM_NONE          0
#define R_ARM_ABS32         2
#define R_ARM_REL32         3
#define R_ARM_THM_CALL      10
#define R_ARM_GLOB_DAT      21
#define R_ARM_JUMP_SLOT     22
#define R_ARM_RELATIVE      23
#define R_ARM_BASE_PREL     25
#define R_ARM_GOT_BREL      26


typedef struct {
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
} Elf32_Sym;

typedef struct {
    uint32_t r_offset;
    uint32_t r_info;
} Elf32_Rel;
// ========== End ELF structures ==========

static uint32_t sys_get_tick(void) {
    static uint32_t tick = 0;
    return tick++;
}

static void sys_delay_ms(uint32_t ms) {
    volatile uint32_t count = ms * 1000;
    while (count--) {
        __asm volatile ("nop");
    }
}

static void sys_putc(char c) {
    putchar(c);
}

static int sys_print(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vprintf(fmt, args);
    va_end(args);
    return ret;
}

static void* sys_malloc(size_t size) {
    (void)size;
    return NULL;
}

static void sys_free(void* ptr) {
    (void)ptr;
}

static void* sys_realloc(void* ptr, size_t size) {
    (void)ptr;
    (void)size;
    return NULL;
}

static const applet_api_t g_applet_api = {
    .version = 1,
    .print = sys_print,
    .putc = sys_putc,
    .get_tick = sys_get_tick,
    .delay_ms = sys_delay_ms,
    .malloc = sys_malloc,
    .free = sys_free,
    .realloc = sys_realloc,
};

#define APPLET_MAX_SIZE 4096
static uint8_t applet_memory[APPLET_MAX_SIZE] __attribute__((aligned(4)));

static void* loader_lookup_symbol(const char* name) {
    if (strcmp(name, "printf") == 0) return (void*)sys_print;
    if (strcmp(name, "malloc") == 0) return (void*)sys_malloc;
    if (strcmp(name, "free") == 0) return (void*)sys_free;
    if (strcmp(name, "realloc") == 0) return (void*)sys_realloc;
    return NULL;
}

static void apply_relocations(uint8_t* load_base, uint32_t link_base, int64_t delta,
                               const Elf32_Rel* rel, uint32_t relsz,
                               const Elf32_Sym* symtab, const char* strtab) {
    uint32_t count = relsz / sizeof(Elf32_Rel);
    for (uint32_t i = 0; i < count; i++) {
        uint32_t r_offset = rel[i].r_offset;
        uint32_t r_info = rel[i].r_info;
        uint32_t r_sym = r_info >> 8;
        uint32_t r_type = r_info & 0xFF;

        uint32_t* target = (uint32_t*)(load_base + (r_offset - link_base));

        switch (r_type) {
            case R_ARM_RELATIVE:
                *target = (uint32_t)((int64_t)*target + delta);
                break;

            case R_ARM_ABS32:
            case R_ARM_GLOB_DAT:
            case R_ARM_JUMP_SLOT: {
                const Elf32_Sym* sym = &symtab[r_sym];
                uint32_t sym_addr;
                if (sym->st_shndx == 0) {
                    const char* sym_name = strtab + sym->st_name;
                    void* resolved = loader_lookup_symbol(sym_name);
                    if (resolved == NULL) {
                        printf("Warning: unresolved symbol '%s'\n", sym_name);
                        continue;
                    }
                    sym_addr = (uint32_t)resolved;
                } else {
                    sym_addr = (uint32_t)((int64_t)sym->st_value + delta);
                }
                *target = sym_addr;
                break;
            }

            default:
                break;
        }
    }
}

static int load_applet(const uint8_t* data, size_t size) {
    if (size > APPLET_MAX_SIZE) {
        printf("Error: Applet too large (%zu > %d bytes)\n", size, APPLET_MAX_SIZE);
        return -1;
    }
    
    printf("Loading applet (%zu bytes)...\n", size);
    memcpy(applet_memory, data, size);
    
    applet_header_t* header = (applet_header_t*)applet_memory;
    
    if (header->magic != APPLET_MAGIC) {
        printf("Error: Invalid applet magic (expected 0x%08lX, got 0x%08lX)\n", 
               (uint32_t)APPLET_MAGIC, (uint32_t)header->magic);
        return -1;
    }
    
    if (header->version != APPLET_VERSION) {
        printf("Error: Unsupported applet version (expected %d, got %lu)\n",
               APPLET_VERSION, (unsigned long)header->version);
        return -1;
    }
    
    printf("Applet magic: 0x%08lX OK\n", (uint32_t)header->magic);
    printf("Applet version: %lu\n", (unsigned long)header->version);
    
    const uint32_t link_base = APPLET_LINK_ADDR;
    uint32_t load_base = (uint32_t)applet_memory;
    int64_t delta = (int64_t)load_base - (int64_t)link_base;
    
    printf("Link base: 0x%08lX\n", (unsigned long)link_base);
    printf("Load base: 0x%08lX\n", (unsigned long)load_base);
    printf("Delta: %ld\n", (long)delta);

    printf("=====================================\n");
    void* dynamic_load = applet_memory + sizeof(*header);
    printf("Header size: 0x%08lX\n", (unsigned long)sizeof(*header));
    printf("Dynamic section addr: 0x%08lX\n", (unsigned long)dynamic_load);

    const Elf32_Dyn* dyn = (const Elf32_Dyn*)dynamic_load;

    const Elf32_Rel* rel = NULL;
    uint32_t relsz = 0;
    const Elf32_Rel* jmprel = NULL;
    uint32_t jmprelsz = 0;
    const Elf32_Sym* symtab = NULL;
    const char* strtab = NULL;

    for (int i = 0; dyn[i].d_tag != DT_NULL; i++) {
        printf(
                "Dynamic: tag %s(%ld) = 0x%08lX\n",
                dynamic_tag_string(dyn[i].d_tag),
                (unsigned long)dyn[i].d_tag,
                (unsigned long)dyn[i].d_un.d_val
              );

        void *addr = (void *)(uint32_t)(dyn[i].d_un.d_ptr + delta);
        switch ((uint32_t)dyn[i].d_tag) {
            case DT_REL:      rel = (const Elf32_Rel*)addr; break;
            case DT_RELSZ:    relsz = dyn[i].d_un.d_val; break;
            case DT_JMPREL:   jmprel = (const Elf32_Rel*)addr; break;
            case DT_PLTRELSZ: jmprelsz = dyn[i].d_un.d_val; break;
            case DT_SYMTAB:   symtab = (const Elf32_Sym*)addr; break;
            case DT_STRTAB:   strtab = (const char*)addr; break;
        }
    }

    if (rel != NULL && symtab != NULL && strtab != NULL) {
        printf("------------\n");
        printf("Applying .rel.dyn (%lu bytes)...\n", (unsigned long)relsz);
        apply_relocations(applet_memory, link_base, delta, rel, relsz, symtab, strtab);
    }
    if (jmprel != NULL && symtab != NULL && strtab != NULL) {
        printf("------------\n");
        printf("Applying .rel.plt (%lu bytes)...\n", (unsigned long)jmprelsz);
        apply_relocations(applet_memory, link_base, delta, jmprel, jmprelsz, symtab, strtab);
    }
    
    printf("=====================================\n");
    printf("Entry: 0x%08lX\n", (unsigned long)header->entry);
    printf("Name: %s\n", header->name);
    
    printf("Executing applet...\n");
    
    uint32_t result = header->entry(&g_applet_api, NULL);
    
    printf("=====================================\n");
    printf("Applet returned: %lu\n", (unsigned long)result);
    
    return 0;
}

// Embedded applet binary (generated by Makefile)
extern unsigned char out_applet_loader_applets_sample_applet_bin[];
extern const unsigned int out_applet_loader_applets_sample_applet_bin_len;

extern unsigned char out_applet_loader_applets_counter_applet_bin[];
extern const unsigned int out_applet_loader_applets_counter_applet_bin_len;

int main(void) {
    stdout_init();
    
    printf("\n========================================\n");
    printf("MPS2-AN385 Minimal Applet Loader\n");
    printf("========================================\n\n");
    
    load_applet(out_applet_loader_applets_sample_applet_bin, out_applet_loader_applets_sample_applet_bin_len);
    load_applet(out_applet_loader_applets_counter_applet_bin, out_applet_loader_applets_counter_applet_bin_len);

    printf("\n========================================\n");
    printf("all success!\n");
    printf("========================================\n");
    
    while (1) {
        __asm volatile ("wfi");
    }
}
