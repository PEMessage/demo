#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

typedef struct {
    char *buffer;     // Pointer to the buffer memory
    size_t size;      // Total size of the buffer
    size_t head;       // Index of the next byte to read
    size_t tail;       // Index of the next byte to write
    bool full;         // Flag indicating if buffer is full
} RingBuffer;

// Initialize a new ring buffer with the given size
RingBuffer* ringbuffer_init(size_t size) {
    if(size == 0 ) return NULL;

    RingBuffer *rb = (RingBuffer *)malloc(sizeof(RingBuffer));
    if (!rb) return NULL;
    
    rb->buffer = (char *)malloc(size);
    if (!rb->buffer) {
        free(rb);
        return NULL;
    }
    
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
    rb->full = false;
    return rb;
}

// Free the ring buffer and its resources
void ringbuffer_free(RingBuffer *rb) {
    if (rb) {
        free(rb->buffer);
        free(rb);
    }
}

// Get the number of bytes available to read
size_t ringbuffer_available_read(const RingBuffer *rb) {
    if (rb->full) return rb->size;
    return (rb->tail >= rb->head) ? (rb->tail - rb->head) : (rb->size - rb->head + rb->tail);
}

// Get the number of bytes available to write
size_t ringbuffer_available_write(const RingBuffer *rb) {
    return rb->size - ringbuffer_available_read(rb);
}

// Read data from the ring buffer
size_t ringbuffer_read(RingBuffer *rb, char *data, size_t count) {
    if (!rb || !data || count == 0) return 0;
    
    size_t available = ringbuffer_available_read(rb);
    if (available == 0) return 0;
    
    if (count > available) count = available;
    
    size_t bytes_to_read = count;
    size_t bytes_read = 0;
    
    // First possible contiguous block (from head to end of buffer or wrap around)
    size_t chunk = (rb->head + count > rb->size) ? (rb->size - rb->head) : count;
    memcpy(data, rb->buffer + rb->head, chunk);
    
    bytes_read += chunk;
    rb->head = (rb->head + chunk) % rb->size;
    
    // Second possible block (if we wrapped around)
    if (bytes_read < count) {
        size_t remaining = count - bytes_read;
        memcpy(data + bytes_read, rb->buffer + rb->head, remaining);
        rb->head = (rb->head + remaining) % rb->size;
        bytes_read += remaining;
    }
    
    rb->full = false;
    return bytes_read;
}

// Write data to the ring buffer
size_t ringbuffer_write(RingBuffer *rb, const char *data, size_t count) {
    if (!rb || !data || count == 0) return 0;
    
    size_t available = ringbuffer_available_write(rb);
    if (available == 0) return 0;
    
    if (count > available) count = available;
    
    size_t bytes_to_write = count;
    size_t bytes_written = 0;
    
    // First possible contiguous block (from tail to end of buffer or wrap around)
    size_t chunk = (rb->tail + count > rb->size) ? (rb->size - rb->tail) : count;
    memcpy(rb->buffer + rb->tail, data, chunk);
    
    bytes_written += chunk;
    rb->tail = (rb->tail + chunk) % rb->size;
    
    // Second possible block (if we wrapped around)
    if (bytes_written < count) {
        size_t remaining = count - bytes_written;
        memcpy(rb->buffer + rb->tail, data + bytes_written, remaining);
        rb->tail = (rb->tail + remaining) % rb->size;
        bytes_written += remaining;
    }
    
    // Check if buffer is now full
    if (bytes_written > 0 && rb->tail == rb->head) {
        rb->full = true;
    }
    
    return bytes_written;
}

// Get a pointer to the element at position index (0-based, relative to head)
// Returns NULL if index is out of bounds
const char* ringbuffer_at(const RingBuffer *rb, size_t index) {
    if (!rb || index >= ringbuffer_available_read(rb)) {
        return NULL;
    }
    
    size_t actual_index = (rb->head + index) % rb->size;
    return &rb->buffer[actual_index];
}

// Get a mutable pointer to the element at position index (0-based, relative to head)
// Returns NULL if index is out of bounds
char* ringbuffer_at_mut(RingBuffer *rb, size_t index) {
    if (!rb || index >= ringbuffer_available_read(rb)) {
        return NULL;
    }
    
    size_t actual_index = (rb->head + index) % rb->size;
    return &rb->buffer[actual_index];
}




void test_basic_operations() {
    printf("=== Testing basic operations ===\n");
    RingBuffer *rb = ringbuffer_init(10);
    assert(rb != NULL);

    // Test initial state
    assert(ringbuffer_available_read(rb) == 0);
    assert(ringbuffer_available_write(rb) == 10);

    // Test writing
    const char *test_data = "Hello";
    size_t written = ringbuffer_write(rb, test_data, strlen(test_data));
    assert(written == 5);
    assert(ringbuffer_available_read(rb) == 5);
    assert(ringbuffer_available_write(rb) == 5);

    // Test reading
    char read_buf[10] = {0};
    size_t read = ringbuffer_read(rb, read_buf, sizeof(read_buf));
    assert(read == 5);
    assert(strcmp(read_buf, "Hello") == 0);
    assert(ringbuffer_available_read(rb) == 0);
    assert(ringbuffer_available_write(rb) == 10);

    ringbuffer_free(rb);
    printf("Basic operations test passed!\n\n");
}

void test_wrap_around() {
    printf("=== Testing wrap around ===\n");
    RingBuffer *rb = ringbuffer_init(5);
    assert(rb != NULL);

    // Write 3 bytes (head=0, tail=3)
    assert(ringbuffer_write(rb, "ABC", 3) == 3);

    // Read 2 bytes (head=2, tail=3)
    char buf[5] = {0};
    assert(ringbuffer_read(rb, buf, 2) == 2);
    assert(strncmp(buf, "AB", 2) == 0);

    // Write 3 more bytes (should wrap around)
    // Buffer: [C, D, E, _, _] but head=2, tail=1
    assert(ringbuffer_write(rb, "DE", 2) == 2);
    assert(ringbuffer_write(rb, "F", 1) == 1);  // This should fail (only 1 byte available)

    // Verify contents
    assert(ringbuffer_available_read(rb) == 4);  // C, D, E, F
    assert(ringbuffer_read(rb, buf, 3) == 3);
    assert(strncmp(buf, "CDE", 3) == 0);

    ringbuffer_free(rb);
    printf("Wrap around test passed!\n\n");
}

void test_full_and_empty() {
    printf("=== Testing full and empty conditions ===\n");
    RingBuffer *rb = ringbuffer_init(3);
    assert(rb != NULL);

    // Fill buffer completely
    assert(ringbuffer_write(rb, "12", 2) == 2);
    assert(ringbuffer_write(rb, "3", 1) == 1);
    assert(ringbuffer_available_write(rb) == 0);
    assert(rb->full == true);

    // Try to write when full
    assert(ringbuffer_write(rb, "4", 1) == 0);

    // Read all data
    char buf[4] = {0};
    assert(ringbuffer_read(rb, buf, 3) == 3);
    assert(strcmp(buf, "123") == 0);
    assert(ringbuffer_available_read(rb) == 0);
    assert(rb->full == false);

    // Try to read when empty
    assert(ringbuffer_read(rb, buf, 1) == 0);

    ringbuffer_free(rb);
    printf("Full and empty test passed!\n\n");
}

void test_random_access() {
    printf("=== Testing random access ===\n");
    RingBuffer *rb = ringbuffer_init(8);
    assert(rb != NULL);

    // Write some data
    assert(ringbuffer_write(rb, "ABCDEF", 6) == 6);

    // Test ringbuffer_at
    const char *ptr = ringbuffer_at(rb, 2);
    assert(ptr != NULL);
    assert(*ptr == 'C');

    // Test ringbuffer_at_mut
    char *mut_ptr = ringbuffer_at_mut(rb, 3);
    assert(mut_ptr != NULL);
    *mut_ptr = 'X';  // Change 'D' to 'X'

    // Verify change
    char buf[7] = {0};
    assert(ringbuffer_read(rb, buf, 6) == 6);
    assert(strcmp(buf, "ABCXEF") == 0);

    // Test out of bounds access
    assert(ringbuffer_at(rb, 10) == NULL);
    assert(ringbuffer_at_mut(rb, 10) == NULL);

    ringbuffer_free(rb);
    printf("Random access test passed!\n\n");
}

void test_edge_cases() {
    printf("=== Testing edge cases ===\n");
    // Test zero size buffer
    RingBuffer *rb = ringbuffer_init(0);
    assert(rb == NULL);

    // Test NULL pointer handling
    assert(ringbuffer_write(NULL, "test", 4) == 0);
    assert(ringbuffer_read(NULL, NULL, 10) == 0);

    // Test with NULL data pointer (should be handled safely)
    rb = ringbuffer_init(10);
    assert(ringbuffer_write(rb, NULL, 5) == 0);
    assert(ringbuffer_read(rb, NULL, 5) == 0);
    ringbuffer_free(rb);

    printf("Edge cases test passed!\n\n");
}

int main() {
    test_basic_operations();
    test_wrap_around();
    test_full_and_empty();
    test_random_access();
    test_edge_cases();

    printf("All tests passed successfully!\n");
    return 0;
}
