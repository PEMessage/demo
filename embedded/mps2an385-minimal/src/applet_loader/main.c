#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include "applet_api.h"

extern int stdout_init(void);

// ========== Applet loader status codes ==========
// One unified error namespace, prefixed AL_ (applet_loader). AL_OK == 0.
typedef enum {
    AL_OK = 0,

    // load_applet
    AL_ERR_TOO_BIG,          // image larger than APPLET_MAX_SIZE
    AL_ERR_BAD_MAGIC,        // applet_header.magic mismatch
    AL_ERR_BAD_VERSION,      // applet_header.version mismatch
    AL_ERR_BAD_BSS,          // BSS range lies outside the image

    // .dynamic parsing
    AL_ERR_BAD_ENTRY_SIZE,   // DT_SYMENT / DT_RELENT != the loader's structs
    AL_ERR_NO_DT_NULL,       // .dynamic is not terminated by DT_NULL

    // relocation
    AL_ERR_BAD_TABLE,        // relocation/symbol/string table outside the image
    AL_ERR_SYM_RANGE,        // r_sym index out of range
    AL_ERR_SYM_NAME,         // st_name offset out of range
    AL_ERR_RELOC_TARGET,     // relocation target outside the image
    AL_ERR_UNHANDLED_RELOC,  // relocation type the loader does not implement
    AL_ERR_UNRESOLVED_SYM,   // external symbol not found (soft; caller may skip)
} al_status_t;

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
    const uint32_t*  hash;    // DT_HASH (SYSV); hash[1] = number of .dynsym entries
    uint32_t symcount;        // = hash[1] (nchain); bounds every r_sym
    uint32_t strsz;           // DT_STRSZ; bounds every st_name
    uint32_t syment;          // DT_SYMENT: size of one .dynsym entry
    uint32_t relent;          // DT_RELENT: size of one relocation entry
} applet_dyn_info_t;

// Everything the relocation pass needs. image_base/image_limit bound the copied
// applet, so the loader never reads or writes outside its own buffer.
typedef struct {
    int64_t          delta;
    const Elf32_Sym* symtab;
    uint32_t         symcount;
    const char*      strtab;
    uint32_t         strsz;
    uintptr_t        image_base;
    uintptr_t        image_limit;
} reloc_ctx_t;

static uint32_t reloc_sym_index(uint32_t r_info) { return r_info >> 8; }
static uint32_t reloc_type(uint32_t r_info)      { return r_info & 0xFF; }

// Is [addr, addr+len) fully inside the loaded applet image?
// len is uint64_t so a huge (attacker-controlled) length cannot wrap around.
static int in_image(const reloc_ctx_t* ctx, uintptr_t addr, uint64_t len) {
    return addr >= ctx->image_base
        && addr <= ctx->image_limit
        && len <= (uint64_t)(ctx->image_limit - addr);
}

// Address to patch (link-time offset rebased), or NULL if outside the image.
static uint32_t* reloc_slot(const reloc_ctx_t* ctx, const Elf32_Rel* r) {
    uintptr_t addr = (uintptr_t)((int64_t)r->r_offset + ctx->delta);
    return in_image(ctx, addr, sizeof(uint32_t)) ? (uint32_t*)addr : NULL;
}

// Resolve one symbol to its runtime address.
// AL_OK on success; AL_ERR_UNRESOLVED_SYM for a missing external symbol (soft;
// the caller may leave the slot untouched); another AL_ERR_* on bad input.
static al_status_t resolve_symbol(const reloc_ctx_t* ctx, uint32_t sym_index,
                                  uint32_t* addr_out) {
    if (sym_index >= ctx->symcount) {
        printf("Error: symbol index %lu out of range (%lu)\n",
               (unsigned long)sym_index, (unsigned long)ctx->symcount);
        return AL_ERR_SYM_RANGE;
    }
    const Elf32_Sym* sym = &ctx->symtab[sym_index];

    if (sym->st_shndx != SHN_UNDEF) {                 // defined by this applet
        *addr_out = (uint32_t)((int64_t)sym->st_value + ctx->delta);
        printf("| local symbol[%lu]: 0x%08lX -> 0x%08lX\n",
               (unsigned long)sym_index,
               (unsigned long)sym->st_value, (unsigned long)*addr_out);
        return AL_OK;
    }

    if (sym->st_name >= ctx->strsz) {
        printf("Error: symbol[%lu] name offset %lu out of range (%lu)\n",
               (unsigned long)sym_index, (unsigned long)sym->st_name,
               (unsigned long)ctx->strsz);
        return AL_ERR_SYM_NAME;
    }
    const char* name = ctx->strtab + sym->st_name;
    void* resolved = loader_lookup_symbol(name);      // defined by the host
    printf("| external symbol[%lu] '%s' -> %p\n",
           (unsigned long)sym_index, name, resolved);

    if (resolved == NULL) {
        printf("Warning: unresolved symbol '%s'\n", name);
        return AL_ERR_UNRESOLVED_SYM;
    }
    *addr_out = (uint32_t)resolved;
    return AL_OK;
}

// Apply one relocation. AL_OK on success (a soft, unresolved symbol is also
// "success" — the slot is just left untouched); another AL_ERR_* on bad input.
static al_status_t apply_one_reloc(const reloc_ctx_t* ctx, uint32_t index, const Elf32_Rel* r) {
    uint32_t r_info = r->r_info;
    uint32_t type   = reloc_type(r_info);

    printf("[%lu] %p: %s offset = 0x%08lX\n",
           (unsigned long)index, (const void*)r,
           arm_reloc_tag_string(type), (unsigned long)r->r_offset);

    uint32_t* slot = reloc_slot(ctx, r);
    if (slot == NULL) {
        printf("Error: relocation target 0x%08lX is outside the applet image\n",
               (unsigned long)r->r_offset);
        return AL_ERR_RELOC_TARGET;
    }

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
            al_status_t st = resolve_symbol(ctx, reloc_sym_index(r_info), &sym_addr);
            if (st == AL_ERR_UNRESOLVED_SYM) {
                break;                                // soft: leave slot untouched
            }
            if (st != AL_OK) {
                return st;                            // hard: propagate
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
            al_status_t st = resolve_symbol(ctx, reloc_sym_index(r_info), &sym_addr);
            if (st == AL_ERR_UNRESOLVED_SYM) {
                break;                                // soft: leave slot untouched
            }
            if (st != AL_OK) {
                return st;                            // hard: propagate
            }
            *slot = sym_addr;
            printf("| %s: S -> 0x%08lX\n",
                   arm_reloc_tag_string(type), (unsigned long)*slot);
            break;
        }

        default:
            printf("Error: unhandled relocation type %lu (%s)\n",
                   (unsigned long)type, arm_reloc_tag_string(type));
            return AL_ERR_UNHANDLED_RELOC;
    }
    return AL_OK;
}

// The .dynsym/.dynstr tables must lie inside the image, otherwise symbol
// lookups could read arbitrary memory.
static al_status_t validate_symbol_tables(const reloc_ctx_t* ctx) {
    if (ctx->symtab != NULL &&
        !in_image(ctx, (uintptr_t)ctx->symtab,
                  (uint64_t)ctx->symcount * sizeof(Elf32_Sym))) {
        printf("Error: symbol table is outside the applet image\n");
        return AL_ERR_BAD_TABLE;
    }
    if (ctx->strtab != NULL && !in_image(ctx, (uintptr_t)ctx->strtab, ctx->strsz)) {
        printf("Error: string table is outside the applet image\n");
        return AL_ERR_BAD_TABLE;
    }
    return AL_OK;
}

static al_status_t apply_single_relocs(const reloc_ctx_t* ctx, const char* name,
                                       const Elf32_Rel* rel, uint32_t relsz) {
    if (rel == NULL || relsz == 0) {
        return AL_OK;                             // table not present
    }
    if (ctx->symtab == NULL || ctx->strtab == NULL) {
        printf("Error: %s present but no symbol/string table\n", name);
        return AL_ERR_BAD_TABLE;
    }
    if (!in_image(ctx, (uintptr_t)rel, relsz)) {
        printf("Error: %s table (0x%08lX + %lu) is outside the applet image\n",
               name, (unsigned long)(uintptr_t)rel, (unsigned long)relsz);
        return AL_ERR_BAD_TABLE;
    }

    printf("------------\n");
    printf("Applying %s (%lu bytes)...\n", name, (unsigned long)relsz);

    uint32_t count = relsz / sizeof(Elf32_Rel);
    for (uint32_t i = 0; i < count; i++) {
        al_status_t st = apply_one_reloc(ctx, i, &rel[i]);
        if (st != AL_OK) {
            return st;
        }
    }
    return AL_OK;
}

static al_status_t apply_relocations(const applet_dyn_info_t* dyn,
                                     uintptr_t image_base, uintptr_t image_limit) {
    const reloc_ctx_t ctx = {
        .delta       = (int64_t)image_base - (int64_t)APPLET_LINK_ADDR,
        .symtab      = dyn->symtab,
        .symcount    = dyn->symcount,
        .strtab      = dyn->strtab,
        .strsz       = dyn->strsz,
        .image_base  = image_base,
        .image_limit = image_limit,
    };

    al_status_t st = validate_symbol_tables(&ctx);
    if (st != AL_OK) {
        return st;
    }
    st = apply_single_relocs(&ctx, ".rel.dyn", dyn->rel, dyn->relsz);
    if (st != AL_OK) {
        return st;
    }
    return apply_single_relocs(&ctx, ".rel.plt", dyn->jmprel, dyn->jmprelsz);
}

// .dynamic has no length of its own; it ends at the DT_NULL entry. This bound
// stops a truncated/corrupt section from being read out of bounds.
#define MAX_DYNAMIC_ENTRIES 64

// Parse the .dynamic tag list into *info.
// Returns 0 on success, -1 if the metadata is incompatible with this loader.
static al_status_t parse_dynamic_section(const Elf32_Dyn* dyn, int64_t delta,
                                         applet_dyn_info_t* info) {
    memset(info, 0, sizeof(*info));

    int terminated = 0;
    for (int i = 0; i < MAX_DYNAMIC_ENTRIES; i++) {
        if (dyn[i].d_tag == DT_NULL) {
            terminated = 1;                       // normal end of .dynamic
            break;
        }

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
            case DT_HASH:     info->hash = (const uint32_t*)addr; break;
            case DT_STRSZ:    info->strsz = val; break;
            case DT_SYMENT:
                info->syment = val;
                if (val != sizeof(Elf32_Sym)) {
                    printf("Error: DT_SYMENT=%lu but sizeof(Elf32_Sym)=%lu\n",
                           (unsigned long)val, (unsigned long)sizeof(Elf32_Sym));
                    return AL_ERR_BAD_ENTRY_SIZE;
                }
                break;
            case DT_RELENT:
                info->relent = val;
                if (val != sizeof(Elf32_Rel)) {
                    printf("Error: DT_RELENT=%lu but sizeof(Elf32_Rel)=%lu\n",
                           (unsigned long)val, (unsigned long)sizeof(Elf32_Rel));
                    return AL_ERR_BAD_ENTRY_SIZE;
                }
                break;
        }
    }

    if (!terminated) {
        printf("Error: .dynamic is not terminated by DT_NULL\n");
        return AL_ERR_NO_DT_NULL;
    }

    // .dynsym has no count tag of its own; the SYSV hash's 2nd word (nchain)
    // is its length, and bounds every r_sym we will use.
    if (info->hash != NULL) {
        info->symcount = info->hash[1];
    }
    return AL_OK;
}

static al_status_t load_applet(const uint8_t* data, size_t size) {
    if (size > APPLET_MAX_SIZE) {
        printf("Error: Applet too large (%zu > %d bytes)\n", size, APPLET_MAX_SIZE);
        return AL_ERR_TOO_BIG;
    }
    
    // some background bytes to help identify bug
    memset(applet_memory, 0xAE, sizeof(applet_memory));

    printf("Loading applet (%zu bytes)...\n", size);
    memcpy(applet_memory, data, size);
    
    applet_header_t* header = (applet_header_t*)applet_memory;
    
    if (header->magic != APPLET_MAGIC) {
        printf("Error: Invalid applet magic (expected 0x%08lX, got 0x%08lX)\n", 
               (uint32_t)APPLET_MAGIC, (uint32_t)header->magic);
        return AL_ERR_BAD_MAGIC;
    }
    
    if (header->version != APPLET_VERSION) {
        printf("Error: Unsupported applet version (expected %d, got %lu)\n",
               APPLET_VERSION, (unsigned long)header->version);
        return AL_ERR_BAD_VERSION;
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
    al_status_t status = parse_dynamic_section((const Elf32_Dyn*)dynamic_load, delta, &dyn_info);
    if (status != AL_OK) {
        printf("Error: bad .dynamic section, refusing to load\n");
        return status;
    }

    status = apply_relocations(&dyn_info, (uintptr_t)applet_memory,
                               (uintptr_t)applet_memory + APPLET_MAX_SIZE);
    if (status != AL_OK) {
        printf("Error: relocation failed, refusing to load\n");
        return status;
    }
    
    // NOTICE:
    // .rel.dyn's  R_ARM_ABS32 will modify header->bss_start/bss_end,
    // so header->bss_start / end only accessiable after `apply_relocations`
    printf("=====================================\n");
    printf("BSS: 0x%08X -  0x%08X\n", (uintptr_t)header->bss_start, (uintptr_t)header->bss_end);
    if (!(header->bss_start >= (void *)applet_memory && header->bss_end <= (void *)(applet_memory + APPLET_MAX_SIZE))) {
        printf("BSS: Exceed limit\n");
        return AL_ERR_BAD_BSS;
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
    
    return AL_OK;
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
