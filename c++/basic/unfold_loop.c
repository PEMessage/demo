#include <stdio.h>
#include <string.h>

// Traditional copy using a simple loop
void simple_copy(char* to, char* from, int count) {
    do {
        *to++ = *from++;
    } while (--count > 0);
}

// Duff's device version of copy
void duff_copy(char* to, char* from, int count) {
    int n = (count + 7) / 8;  // Round up to account for partial blocks
    
    switch (count % 8) {
        case 0: do { *to++ = *from++;
        case 7:      *to++ = *from++;
        case 6:      *to++ = *from++;
        case 5:      *to++ = *from++;
        case 4:      *to++ = *from++;
        case 3:      *to++ = *from++;
        case 2:      *to++ = *from++;
        case 1:      *to++ = *from++;
                } while (--n > 0);
    }
}

int main() {
    char src[32] = "This is a test string for Duff!";
    char dest1[32] = {0};
    char dest2[32] = {0};
    
    int count = strlen(src) + 1; // Include null terminator
    
    printf("Original string: %s\n", src);
    
    // Test simple copy
    simple_copy(dest1, src, count);
    printf("Simple copy: %s\n", dest1);
    
    // Test Duff's device copy
    duff_copy(dest2, src, count);
    printf("Duff's copy: %s\n", dest2);
    
    // Verify they match
    if (memcmp(dest1, dest2, count)) {
        printf("Error: Copies don't match!\n");
    } else {
        printf("Success: Both copies match!\n");
    }
    
    return 0;
}
