#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

// -------------------- Configuration & Globals --------------------
#define CANARY_SIZE     4                          // 4 bytes for uint32_t
#define CANARY_MAGIC    0xBEADBEEF                 // fixed magic value

static uint32_t canary_value = CANARY_MAGIC;       // canary stored as uint32_t

// -------------------- Helper Functions --------------------
// Initialize canary (fixed, no randomisation)
void init_canary(void) {
    canary_value = CANARY_MAGIC;   // no longer reads from /dev/urandom
}

// Fill canary at the end of the buffer (buffer must have at least len + CANARY_SIZE bytes)
void fill_canary(void *buf, size_t len) {
    memcpy((char*)buf + len, &canary_value, CANARY_SIZE);
}

// Check the canary at the end of the buffer
// Return: 0 if intact, -1 if corrupted
int check_canary(void *buf, size_t len) {
    if (memcmp((char*)buf + len, &canary_value, CANARY_SIZE) != 0) {
        fprintf(stderr, "ERROR: Buffer overflow detected (canary corrupted)\n");
        return -1;
    }
    return 0;
}

// -------------------- Heap Memory Wrappers --------------------
void* my_malloc(size_t size) {
    if (size == 0) return NULL;

    size_t total = sizeof(size_t) + size + CANARY_SIZE;
    void *raw_ptr = malloc(total);
    if (!raw_ptr) return NULL;

    // Store the user-requested size
    *(size_t*)raw_ptr = size;

    void *user_ptr = (char*)raw_ptr + sizeof(size_t);

    // Write canary at the tail
    fill_canary(user_ptr, size);

    return user_ptr;
}

// Actively check the canary of a user pointer
int my_check(void *ptr) {
    if (ptr == NULL) return 0;

    void *raw_ptr = (char*)ptr - sizeof(size_t);
    size_t size = *(size_t*)raw_ptr;

    return check_canary(ptr, size);
}

void my_free(void *ptr) {
    if (ptr == NULL) return;

    if (my_check(ptr) != 0) {
        abort();   // overflow detected, abort program
    }

    void *raw_ptr = (char*)ptr - sizeof(size_t);
    free(raw_ptr);
}

// -------------------- Test main() --------------------
// Macros to intentionally trigger overflows (set to 1 to test)
#define TEST_HEAP_OVERFLOW  0   // change to 1 to test heap overflow
#define TEST_STACK_OVERFLOW 0   // change to 1 to test stack overflow

int main() {
    init_canary();

    // ========== Heap test ==========
    printf("=== Heap test ===\n");
    size_t heap_size = 16;
    char *heap_buf = (char*)my_malloc(heap_size);
    if (!heap_buf) {
        fprintf(stderr, "my_malloc failed\n");
        return 1;
    }

    strcpy(heap_buf, "Hello, heap!");
    printf("Heap buffer content: %s\n", heap_buf);

    if (my_check(heap_buf) == 0) {
        printf("Heap canary is intact.\n");
    }

#if TEST_HEAP_OVERFLOW
    printf("Intentionally overflowing heap buffer...\n");
    heap_buf[heap_size] = 'X';   // corrupt canary
    my_free(heap_buf);           // will abort inside
#else
    my_free(heap_buf);
    printf("Heap buffer freed normally.\n");
#endif

    // ========== Stack array test ==========
    printf("\n=== Stack array test ===\n");
    size_t stack_len = 20;
    char stack_buf[stack_len + CANARY_SIZE];   // reserve space for canary (now 4 bytes)

    strncpy(stack_buf, "Hello, stack!", stack_len);
    fill_canary(stack_buf, stack_len);

    printf("Stack buffer content: %s\n", stack_buf);

    if (check_canary(stack_buf, stack_len) == 0) {
        printf("Stack canary is intact.\n");
    }

#if TEST_STACK_OVERFLOW
    printf("Intentionally overflowing stack buffer...\n");
    stack_buf[stack_len] = 'Y';   // corrupt stack canary
    check_canary(stack_buf, stack_len);   // prints error and returns -1
#else
    printf("Stack buffer safe, canary verified.\n");
#endif

    printf("\nAll tests passed (no overflow triggered).\n");
    return 0;
}
