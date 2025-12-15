#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// nob.h enpower
// ==============================================
#define NOB_ASSERT assert
#define NOB_REALLOC realloc
#define NOB_FREE free
#define NOB_DA_INIT_CAP 4

#ifdef __cplusplus
#define NOB_DECLTYPE_CAST(T) (decltype(T))
#else
#define NOB_DECLTYPE_CAST(T)
#endif // __cplusplus

#define nob_da_reserve(da, expected_capacity)                                              \
    do {                                                                                   \
        if ((expected_capacity) > (da)->capacity) {                                        \
            if ((da)->capacity == 0) {                                                     \
                (da)->capacity = NOB_DA_INIT_CAP;                                          \
            }                                                                              \
            while ((expected_capacity) > (da)->capacity) {                                 \
                (da)->capacity *= 2;                                                       \
            }                                                                              \
            (da)->items = NOB_DECLTYPE_CAST((da)->items)NOB_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items)); \
            NOB_ASSERT((da)->items != NULL && "Buy more RAM lol");                         \
        }                                                                                  \
    } while (0)

#define nob_da_append(da, item)                \
    do {                                       \
        nob_da_reserve((da), (da)->count + 1); \
        (da)->items[(da)->count++] = (item);   \
    } while (0)

#define nob_da_free(da) do { \
    for (size_t i = 0; i < (da).count; i++) { \
        free((da).items[i]); \
    } \
    NOB_FREE((da).items); \
} while (0)

#define nob_da_foreach(Type, it, da) for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} tokens_t;


tokens_t tokenize(const char* str, const char* delim) {
    tokens_t splits = {NULL, 0, 0};
    if (!str || !delim) return splits;

    char* str_copy = strdup(str);
    if (!str_copy) return splits;  // safer than assert in production

    char* saveptr;
    char* token = strtok_r(str_copy, delim, &saveptr);
    while (token) {
        nob_da_append(&splits, strdup(token));
        token = strtok_r(NULL, delim, &saveptr);
    }

    free(str_copy);
    return splits;
}


// Example usage
int main() {
    printf("Running tests...\n");

    // Test 1: With leading delimiter (should give empty first part)
    {
        tokens_t tokens = tokenize("///usr/local/bin///", "/");
        assert(tokens.count == 3);

        // Verify the tokens
        assert(strcmp(tokens.items[0], "usr") == 0);
        assert(strcmp(tokens.items[1], "local") == 0);
        assert(strcmp(tokens.items[2], "bin") == 0);
        printf("\nTest - Path '/':\n");
        nob_da_foreach(char *, it, &tokens) {
            printf("'%s'\n", *it);
        }
        nob_da_free(tokens);
    }


    return 0;
}
