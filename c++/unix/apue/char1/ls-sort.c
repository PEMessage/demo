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

void dynamic_array_free_pointer_array(DynamicArray *array, void (*free_item)(void*)) {
    if (free_item) {
        for (size_t i = 0; i < array->count; i++) {
            void *item = *(void**)((char*)array->items + i * array->item_size);
            free_item(item);
        }
    }
    dynamic_array_free(array);
}

int dynamic_array_append_str(DynamicArray *array, const char *str) {
    char *str_copy = strdup(str);
    if (!str_copy) return 0;
    
    int result = dynamic_array_append(array, &str_copy);
    if (!result) free(str_copy);
    return result;
}

/* ====================== */
/* Main Program Logic */
/* ====================== */

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
    const char *tmp ;
    while ((dirp = readdir(dp)) != NULL) {
        tmp = strdup(dirp->d_name);
        if (!dynamic_array_append(&entries, &tmp)) {
            perror("Memory allocation failed");
            dynamic_array_free_pointer_array(&entries, free);
            closedir(dp);
            exit(1);
        }
    }

    // Sort entries
    qsort(entries.items, entries.count, entries.item_size, (int (*)(const void *, const void *))&strcmp);

    // Print sorted entries
    for (size_t i = 0; i < entries.count; i++) {
        char *name = *(char **)((char *)entries.items + i * entries.item_size);
        printf("%s\n", name);
    }

    // Clean up
    dynamic_array_free_pointer_array(&entries, free);
    closedir(dp);

    return 0;
}
