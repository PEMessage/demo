#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// DeepSeek-V3-0324:
//    Please implentment python list(range(...)) in C,
//    something like
//    [0, 3, 6, 9]   == list(range(0, 10 , 3))
//    we only support int

void print_help(const char* program_name) {
    printf("Usage: %s <start> <stop> <step>\n", program_name);
    printf("Generates a list of integers from start to stop (exclusive) with given step.\n");
    printf("Example: %s 0 10 3  =>  [0, 3, 6, 9]\n", program_name);
    printf("Example: %s 5 0 -1  =>  [5, 4, 3, 2, 1]\n", program_name);
}

int* range(int start, int stop, int step, int* length) {
    // Validate step
    if (step == 0) {
        fprintf(stderr, "range() arg 3 must not be zero\n");
        exit(1);
    }
    
    // Calculate the length of the resulting array
    int len = 0;
    if ((step > 0 && start < stop) || (step < 0 && start > stop)) {
        len = (abs(stop - start) + abs(step) - 1) / abs(step);
    }
    
    // Allocate memory for the array
    int* arr = (int*)malloc(len * sizeof(int));
    if (arr == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    // Fill the array
    for (int i = 0; i < len; i++) {
        arr[i] = start + i * step;
    }
    
    *length = len;
    return arr;
}

int main(int argc, char* argv[]) {
    // Check for correct number of arguments
    if (argc != 4) {
        print_help(argv[0]);
        return 1;
    }
    
    // Parse arguments
    int start = atoi(argv[1]);
    int stop = atoi(argv[2]);
    int step = atoi(argv[3]);
    
    // Generate the range
    int len;
    int* arr = range(start, stop, step, &len);
    
    // Print the result
    printf("[");
    for (int i = 0; i < len; i++) {
        printf("%d", arr[i]);
        if (i < len - 1) {
            printf(", ");
        }
    }
    printf("]\n");
    
    // Clean up
    free(arr);
    return 0;
}
