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

// ========== relocation section ==========
// ARM relocation types
typedef struct {
    uint32_t r_offset;
    uint32_t r_info;
} Elf32_Rel;

#define ARM_RELOC_TYPES                                                       \
    X(R_ARM_NONE,          0)                                                 \
    X(R_ARM_ABS32,         2)                                                 \
    X(R_ARM_REL32,         3)                                                 \
    X(R_ARM_THM_CALL,     10)                                                 \
    X(R_ARM_GLOB_DAT,     21)                                                 \
    X(R_ARM_JUMP_SLOT,    22)                                                 \
    X(R_ARM_RELATIVE,     23)                                                 \
    X(R_ARM_BASE_PREL,    25)                                                 \
    X(R_ARM_GOT_BREL,     26)

typedef enum arm_reloc_tag {
#define X(name, value) name = value,
    ARM_RELOC_TYPES
#undef X
    ARM_RELOC_NUM
} arm_reloc_tag_t;

const char* arm_reloc_tag_string(arm_reloc_tag_t tag) {
    switch (tag) {
#define X(name, value) case name: return #name;
        ARM_RELOC_TYPES
#undef X
        default: return "R_ARM_UNKNOW";
    }
}


typedef struct {
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
} Elf32_Sym;

#define SHN_UNDEF 0   // st_shndx value: symbol is undefined in this object

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

typedef struct {
    const Elf32_Rel* rel;
    uint32_t relsz;
    const Elf32_Rel* jmprel;
    uint32_t jmprelsz;
    const Elf32_Sym* symtab;
    const char* strtab;
    uint32_t syment;         // DT_SYMENT: size of one .dynsym entry
    uint32_t relent;         // DT_RELENT: size of one relocation entry
} applet_dyn_info_t;

// Everything the relocation pass needs that is the same for every entry.
typedef struct {
    int64_t          delta;
    const Elf32_Sym* symtab;
    const char*      strtab;
} reloc_ctx_t;

static uint32_t reloc_sym_index(uint32_t r_info) { return r_info >> 8; }
static uint32_t reloc_type(uint32_t r_info)      { return r_info & 0xFF; }

// Address to patch = link-time offset rebased onto the load address.
static uint32_t* reloc_slot(const Elf32_Rel* r, int64_t delta) {
    return (uint32_t*)(uintptr_t)((int64_t)r->r_offset + delta);
}

// Resolve one symbol to its runtime address.
// Returns 1 on success, 0 if it is an unresolved external symbol.
static int resolve_symbol(const reloc_ctx_t* ctx, uint32_t sym_index, uint32_t* addr_out) {
    const Elf32_Sym* sym = &ctx->symtab[sym_index];

    if (sym->st_shndx != SHN_UNDEF) {                 // defined by this applet
        *addr_out = (uint32_t)((int64_t)sym->st_value + ctx->delta);
        printf("| local symbol[%lu]: 0x%08lX -> 0x%08lX\n",
               (unsigned long)sym_index,
               (unsigned long)sym->st_value, (unsigned long)*addr_out);
        return 1;
    }

    const char* name = ctx->strtab + sym->st_name;
    void* resolved = loader_lookup_symbol(name);      // defined by the host
    printf("| external symbol[%lu] '%s' -> %p\n",
           (unsigned long)sym_index, name, resolved);

    if (resolved == NULL) {
        printf("Warning: unresolved symbol '%s'\n", name);
        return 0;
    }
    *addr_out = (uint32_t)resolved;
    return 1;
}

static void apply_one_reloc(const reloc_ctx_t* ctx, uint32_t index, const Elf32_Rel* r) {
    uint32_t r_info = r->r_info;
    uint32_t type   = reloc_type(r_info);
    uint32_t* slot  = reloc_slot(r, ctx->delta);

    printf("[%lu] %p: %s offset = 0x%08lX\n",
           (unsigned long)index, (const void*)r,
           arm_reloc_tag_string(type), (unsigned long)r->r_offset);

    switch (type) {
        case R_ARM_RELATIVE: {
            uint32_t before = *slot;
            *slot = (uint32_t)((int64_t)before + ctx->delta);
            printf("| RELATIVE: 0x%08lX + delta -> 0x%08lX\n",
                   (unsigned long)before, (unsigned long)*slot);
            break;
        }

        case R_ARM_ABS32: {
            // R_ARM_ABS32 = (S + A) | T : A is the addend already stored
            // in the word, so it must be added to the symbol address.
            uint32_t sym_addr;
            if (!resolve_symbol(ctx, reloc_sym_index(r_info), &sym_addr)) {
                break;                                // leave slot untouched
            }

            uint32_t addend = *slot;
            *slot = sym_addr + addend;
            printf("| ABS32: S(0x%08lX) + A(0x%08lX) -> 0x%08lX\n",
                   (unsigned long)sym_addr, (unsigned long)addend,
                   (unsigned long)*slot);
            break;
        }

        case R_ARM_GLOB_DAT:
        case R_ARM_JUMP_SLOT: {
            // GLOB_DAT / JUMP_SLOT: the slot becomes S (no addend).
            uint32_t sym_addr;
            if (!resolve_symbol(ctx, reloc_sym_index(r_info), &sym_addr)) {
                break;                                // leave slot untouched
            }

            *slot = sym_addr;
            printf("| %s: S -> 0x%08lX\n",
                   arm_reloc_tag_string(type), (unsigned long)*slot);
            break;
        }

        default:
            // Unsupported type: leave the word untouched.
            break;
    }
}

static void apply_single_relocs(const reloc_ctx_t* ctx, const char* name,
                                const Elf32_Rel* rel, uint32_t relsz) {
    if (rel == NULL || ctx->symtab == NULL || ctx->strtab == NULL) {
        return;
    }

    printf("------------\n");
    printf("Applying %s (%lu bytes)...\n", name, (unsigned long)relsz);

    uint32_t count = relsz / sizeof(Elf32_Rel);
    for (uint32_t i = 0; i < count; i++) {
        apply_one_reloc(ctx, i, &rel[i]);
    }
}

static void apply_relocations(const applet_dyn_info_t* dyn, int64_t delta) {
    const reloc_ctx_t ctx = {
        .delta  = delta,
        .symtab = dyn->symtab,
        .strtab = dyn->strtab,
    };

    apply_single_relocs(&ctx, ".rel.dyn", dyn->rel, dyn->relsz);
    apply_single_relocs(&ctx, ".rel.plt", dyn->jmprel, dyn->jmprelsz);
}

// TAG - VALUE table
// DT_HASH    0x400003f4     -> .hash
// DT_STRTAB  0x40000384     -> .dynstr
// DT_SYMTAB  0x400002b4     -> .dynsym
// DT_STRSZ   112 / DT_SYMENT 16
// Parse the .dynamic tag list into *info.
// Returns 0 on success, -1 if the metadata is incompatible with this loader.
static int parse_dynamic_section(const Elf32_Dyn* dyn, int64_t delta,
                                 applet_dyn_info_t* info) {
    memset(info, 0, sizeof(*info));

    for (int i = 0; dyn[i].d_tag != DT_NULL; i++) {
        printf(
                "[%d] %p: tag %s(%ld) = 0x%08lX\n",
                i, &dyn[i],
                dynamic_tag_string(dyn[i].d_tag),
                (unsigned long)dyn[i].d_tag,
                (unsigned long)dyn[i].d_un.d_val
              );

        void *addr = (void *)(uint32_t)(dyn[i].d_un.d_ptr + delta);
        uint32_t val = dyn[i].d_un.d_val;
        switch ((uint32_t)dyn[i].d_tag) {
            case DT_REL:      info->rel = (const Elf32_Rel*)addr; break;
            case DT_RELSZ:    info->relsz = val; break;
            case DT_JMPREL:   info->jmprel = (const Elf32_Rel*)addr; break;
            case DT_PLTRELSZ: info->jmprelsz = val; break;
            case DT_SYMTAB:   info->symtab = (const Elf32_Sym*)addr; break;
            case DT_STRTAB:   info->strtab = (const char*)addr; break;
            case DT_SYMENT:
                info->syment = val;
                if (val != sizeof(Elf32_Sym)) {
                    printf("Error: DT_SYMENT=%lu but sizeof(Elf32_Sym)=%lu\n",
                           (unsigned long)val, (unsigned long)sizeof(Elf32_Sym));
                    return -1;
                }
                break;
            case DT_RELENT:
                info->relent = val;
                if (val != sizeof(Elf32_Rel)) {
                    printf("Error: DT_RELENT=%lu but sizeof(Elf32_Rel)=%lu\n",
                           (unsigned long)val, (unsigned long)sizeof(Elf32_Rel));
                    return -1;
                }
                break;
        }
    }
    return 0;
}

static int load_applet(const uint8_t* data, size_t size) {
    if (size > APPLET_MAX_SIZE) {
        printf("Error: Applet too large (%zu > %d bytes)\n", size, APPLET_MAX_SIZE);
        return -1;
    }
    
    // some background bytes to help identify bug
    memset(applet_memory, 0xAE, sizeof(applet_memory));

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

    applet_dyn_info_t dyn_info;
    if (parse_dynamic_section((const Elf32_Dyn*)dynamic_load, delta, &dyn_info) != 0) {
        printf("Error: incompatible dynamic section, refusing to load\n");
        return -2;
    }

    apply_relocations(&dyn_info, delta);
    
    // NOTICE:
    // .rel.dyn's  R_ARM_ABS32 will modify header->bss_start/bss_end,
    // so header->bss_start / end only accessiable after `apply_relocations`
    printf("=====================================\n");
    printf("BSS: 0x%08X -  0x%08X\n", (uintptr_t)header->bss_start, (uintptr_t)header->bss_end);
    if (!(header->bss_start >= (void *)applet_memory && header->bss_end <= (void *)(applet_memory + APPLET_MAX_SIZE))) {
        printf("BSS: Exceed limit\n");
        return -3;
    } else {
        printf("BSS: Within limit\n");
    }
    memset(header->bss_start, 0, header->bss_end - header->bss_start);

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

extern unsigned char out_applet_loader_applets_addend_applet_bin[];
extern const unsigned int out_applet_loader_applets_addend_applet_bin_len;

int main(void) {
    stdout_init();
    
    printf("\n========================================\n");
    printf("MPS2-AN385 Minimal Applet Loader\n");
    printf("========================================\n\n");
    
    load_applet(out_applet_loader_applets_sample_applet_bin, out_applet_loader_applets_sample_applet_bin_len);
    load_applet(out_applet_loader_applets_counter_applet_bin, out_applet_loader_applets_counter_applet_bin_len);
    load_applet(out_applet_loader_applets_addend_applet_bin, out_applet_loader_applets_addend_applet_bin_len);

    printf("\n========================================\n");
    printf("all success!\n");
    printf("========================================\n");
    
    while (1) {
        __asm volatile ("wfi");
    }
}
