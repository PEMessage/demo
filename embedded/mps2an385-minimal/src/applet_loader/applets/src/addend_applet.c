#include "applet_api.h"

// A global buffer referenced through pointers with non-zero addends.
char test_buffer[32] = "0123456789abcdef";

// These initializers must produce R_ARM_ABS32 relocations against test_buffer
// with addends 0, 4 and 8.  They exercise the additive ABS32 semantics:
//     *(word) = S + A
// If the loader drops A (treats ABS32 like GLOB_DAT), all three pointers end
// up equal to test_buffer and the check below fails.
char *offset_ptrs[3] = {
    test_buffer,      // A = 0
    test_buffer + 4,  // A = 4
    test_buffer + 8,  // A = 8
};

static uint32_t addend_applet_main(const applet_api_t* api, void* arg) {
    (void)arg;
    int ok = 1;

    api->print("\n=== R_ARM_ABS32 addend test applet ===\n");
    api->print("test_buffer = %p\n", (void*)test_buffer);

    for (int i = 0; i < 3; i++) {
        char *expected = test_buffer + i * 4;
        int match = (offset_ptrs[i] == expected);
        ok = ok && match;
        api->print("offset_ptrs[%d] = %p  expect %p  %s  -> \"%s\"\n",
                   i,
                   (void*)offset_ptrs[i],
                   (void*)expected,
                   match ? "OK" : "MISMATCH",
                   offset_ptrs[i]);
    }

    api->print("R_ARM_ABS32 addend: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}

APPLET_EXPORT_HEADER("addend_applet", addend_applet_main);
