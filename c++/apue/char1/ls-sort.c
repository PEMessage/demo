#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

/* ====================== */
/* Generic Dynamic Array API */
/* ====================== */

typedef struct {
    void *items;        // Pointer to array of elements
    size_t item_size;   // Size of each element in bytes
    size_t count;       // Number of elements currently in array
    size_t capacity;    // Total allocated capacity
} DynamicArray;

void dynamic_array_init(DynamicArray *array, size_t item_size) {
    array->items = NULL;
    array->item_size = item_size;
    array->count = 0;
    array->capacity = 0;
}

int dynamic_array_append(DynamicArray *array, const void *item) {
    // Grow array if needed
    if (array->count >= array->capacity) {
        size_t new_capacity = array->capacity == 0 ? 16 : array->capacity * 2;
        void *new_items = realloc(array->items, new_capacity * array->item_size);
        if (!new_items) return 0; // failure
        
        array->items = new_items;
        array->capacity = new_capacity;
    }
    
    // Copy the item
    memcpy((char *)array->items + (array->count * array->item_size), 
           item, array->item_size);
    
    array->count++;
    return 1; // success
}

void dynamic_array_free(DynamicArray *array) {
    free(array->items);
    array->items = NULL;
    array->count = 0;
    array->capacity = 0;
}

/* ====================== */
/* String-specific helpers */
/* ====================== */

int dynamic_array_append_str(DynamicArray *array, const char *str) {
    return dynamic_array_append(array, &str);
}


/* ====================== */
/* Main Program Logic */
/* ====================== */

// Comparison function for qsort
static int compare(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

int main(int argc, char *argv[]) {
    DIR *dp;
    struct dirent *dirp;
    DynamicArray entries;

    // Initialize our dynamic array for strings (char pointers)
    dynamic_array_init(&entries, sizeof(char *));

    // Check arguments
    if (argc != 2) {
        fprintf(stderr, "usage: %s directory_name\n", argv[0]);
        exit(1);
    }

    // Open directory
    if ((dp = opendir(argv[1])) == NULL) {
        perror("Error opening directory");
        exit(1);
    }

    // Read directory entries
    while ((dirp = readdir(dp)) != NULL) {
        if (!dynamic_array_append_str(&entries, dirp->d_name)) {
            perror("Memory allocation failed");
            closedir(dp);
            exit(1);
        }
    }

    // Sort entries
    qsort(entries.items, entries.count, entries.item_size, compare);

    // Print sorted entries
    for (size_t i = 0; i < entries.count; i++) {
        char *name = *(char **)((char *)entries.items + i * entries.item_size);
        printf("%s\n", name);
    }

    // Clean up
    dynamic_array_free(&entries);
    closedir(dp);

    return 0;
}
