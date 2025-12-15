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

#define nob_da_free(da) NOB_FREE((da).items)

#define nob_da_foreach(Type, it, da) for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} tokens_t;


char *chop_once(char *s, const char *delim, char **saveptr)
{
    // without this line. split("", ...) will do a chop
    if (s && *s == '\0') return NULL;

    // same as musl
    // See: https://github.com/kraj/musl/blob/ff441c9ddfefbb94e5881ddd5112b24a944dc36c/src/string/strtok_r.c#L5
    if (!s && !(s = *saveptr)) return NULL;

    // Find end of current token: first char in delim, or end of string
    char *token_end = s + strcspn(s, delim);

    // if find end of str, this time is still valid, next call will return NULL
    if (*token_end == '\0') {
        *saveptr = NULL; // set saveptr to NULL mark as end;
    } else {
        *token_end = '\0';     // chop s
        *saveptr = token_end + 1;
    }
    return s;

}

typedef char* (*operator_t)(char *s, const char *delim, char **saveptr);

tokens_t internal(const char* str, const char* delim, operator_t op) {
    tokens_t tokens = {NULL, 0, 0};
    if (!str || !delim) return tokens;

    char* str_copy = strdup(str);
    if (!str_copy) return tokens;  // safer than assert in production

    char* saveptr;
    for (
            char * token = op(str_copy, delim, &saveptr);
            token;
            token = op(NULL, delim, &saveptr)
        ) {
        // foreach strtok_r like function
        nob_da_append(&tokens, strdup(token));
    }

    free(str_copy);
    return tokens;
}

tokens_t tokenize(const char* str, const char* delim) {
    return internal(str, delim, strtok_r);
}

tokens_t split(const char* str, const char* delim) {
    return internal(str, delim, chop_once);
}

// Example usage
int main() {
    printf("Running tests...\n");

    // Test 1: With leading delimiter (should give empty first part)
    {
        printf("=== Test tokenize:\n");
        tokens_t tokens = tokenize("///usr/local/bin///", "/");
        assert(tokens.count == 3);
        nob_da_foreach(char *, it, &tokens) {
            printf("'%s'\n", *it);
            free(*it);
        }
        nob_da_free(tokens);
    }

    {
        printf("=== Test empty tokenize:\n");
        tokens_t tokens = tokenize("", "/");
        assert(tokens.count == 0);

        nob_da_foreach(char *, it, &tokens) {
            printf("'%s'\n", *it);
            free(*it);
        }
        nob_da_free(tokens);
    }


    {
        printf("=== Test split:\n");
        tokens_t tokens = split("/usr/local/bin/", "/");
        assert(tokens.count == 5);
        nob_da_foreach(char *, it, &tokens) {
            printf("'%s'\n", *it);
            free(*it);
        }
        nob_da_free(tokens);
    }

    {
        tokens_t tokens = split("", "/");
        assert(tokens.count == 0);
        printf("=== Test split empty\n");
        nob_da_foreach(char *, it, &tokens) {
            printf("'%s'\n", *it);
            free(*it);
        }
        nob_da_free(tokens);
    }

    return 0;
}
