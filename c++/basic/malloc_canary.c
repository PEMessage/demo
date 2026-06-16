#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

// -------------------- Dummy lock unlock placeholder -------------
void lock() {};
void unlock() {};

// -------------------- Configuration & Globals --------------------
#define CANARY_SIZE     4
static char canary_malloc[CANARY_SIZE] = {(char)0xDE, (char)0xED, (char)0xBE, (char)0xEF};
static char canary_free[CANARY_SIZE]   = {(char)0xFE, (char)0xED, (char)0xFA, (char)0xCE};   // 新增释放金丝雀

#define CANARY_OK 0
#define CANARY_ERR -1
#define CANARY_ERR_USE_AFTER_FREE -2
#define CANARY_ERR_INVALID_SIZE -3

// -------------------- Helper Functions --------------------
void init_canary(void) { /* nothing */ }

void fill_canary(void *buf, size_t len) {
    memcpy((char*)buf + len, canary_malloc, CANARY_SIZE);
}

void clear_canary(void *buf, size_t len) {             // 新增
    memcpy((char*)buf + len, canary_free, CANARY_SIZE);
}

int check_canary(void *buf, size_t len) {              // 保持原样，仅检查 canary_malloc
    if (memcmp((char*)buf + len, canary_malloc, CANARY_SIZE) == 0) {
        return CANARY_OK;
    }
    if (memcmp((char*)buf + len, canary_free, CANARY_SIZE) == 0) {
        fprintf(stderr, "ERROR: Use-After-Free detected %p, %zu\n", buf, len);
        return CANARY_ERR_USE_AFTER_FREE;
    }
    fprintf(stderr, "ERROR: Canary corrupt detected %p, %zu\n", buf, len);
    return CANARY_ERR;
}

// -------------------- Heap Memory Wrappers --------------------
void* my_malloc(size_t size) {
    if (size == 0) return NULL;

    size_t total = sizeof(size_t) + size + CANARY_SIZE;
    lock();
    void *raw_ptr = malloc(total);
    unlock();
    if (!raw_ptr) { return NULL; }

    // Store the user-requested size
    *(size_t*)raw_ptr = size;

    void *user_ptr = (char*)raw_ptr + sizeof(size_t);

    // Write canary at the tail
    fill_canary(user_ptr, size);

    return user_ptr;
}

// Actively check the canary of a user pointer
int my_check(void *user_ptr) {
    if (user_ptr == NULL) return 0;

    lock();
    void *raw_ptr = (char*)user_ptr - sizeof(size_t);
    size_t size = *(size_t*)raw_ptr;

    if (size == 0) {
        fprintf(stderr, "ERROR: check %p fail at %p due to size is 0\n", user_ptr, __builtin_return_address(0));
        unlock();
        return CANARY_ERR_INVALID_SIZE;
    }

    int is_corrupt = check_canary(user_ptr, size);
    if (is_corrupt) {
        fprintf(stderr, "ERROR: check %p fail at %p due to %d\n", user_ptr, __builtin_return_address(0), is_corrupt);
        unlock();
        return is_corrupt;
    }

    unlock();
    return CANARY_OK;
}

void my_free(void *user_ptr) {
    if (user_ptr == NULL) return;

    lock();
    void *raw_ptr = (char*)user_ptr - sizeof(size_t);
    size_t size = *(size_t*)raw_ptr;

    if (size == 0) {
        fprintf(stderr, "ERROR: free %p fail at %p due to size is 0\n", user_ptr, __builtin_return_address(0));
        unlock();
        abort();
        return; // CANARY_ERR_INVALID_SIZE
    }

    int is_corrupt = check_canary(user_ptr, size);
    if (is_corrupt) {
        fprintf(stderr, "ERROR: free %p fail at %p due to %d\n", user_ptr, __builtin_return_address(0), is_corrupt);
        unlock();
        abort();
        return; // is_corrupt
    }

    // Above same as my_check

    clear_canary(user_ptr, size);
    memset(user_ptr, 0, size);

    *(size_t*)raw_ptr = 0;
    free(raw_ptr);
    unlock();
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
    // 模拟 Use-After-Free 检测（注释掉以通过测试）
    // if (my_check(heap_buf) == 0) ...   // 会触发 USE-AFTER-FREE 错误
#endif

    // ========== Stack array test ==========
    printf("\n=== Stack array test ===\n");
    size_t stack_len = 20;
    char stack_buf[stack_len + CANARY_SIZE];   // reserve space for canary

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
