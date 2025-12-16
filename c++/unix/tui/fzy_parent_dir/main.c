#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include "tty.h"
#include "config.h"


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

// Split
// ====================================
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

tokens_t split(const char* str, const char* delim) {
    tokens_t tokens = {NULL, 0, 0};
    if (!str || !delim) return tokens;

    char* str_copy = strdup(str);
    if (!str_copy) return tokens;  // safer than assert in production

    char* saveptr;
    for (
            char * token = chop_once(str_copy, delim, &saveptr);
            token;
            token = chop_once(NULL, delim, &saveptr)
        ) {
        // foreach strtok_r like function
        nob_da_append(&tokens, strdup(token));
    }

    free(str_copy);
    return tokens;
}


// Helper macro or function
// ====================================

#define ARRAY_SIZE(a) (sizeof((a)) / sizeof((a[0])))
#define ARRAY_INDEX(ptr, array) ((size_t)((void *)(ptr) - (void *)(array)) / sizeof((array)[0]))

// Key sequence definitions
// ====================================
#define CTRL_C "\x03"

#define UP_ARROW "\x1b[A"
#define DOWN_ARROW "\x1b[B"
#define RIGHT_ARROW "\x1b[C"
#define LEFT_ARROW "\x1b[D"

// ALT+arrow keys
// ALT modifier = 3
#define ALT_UP_ARROW "\x1b[1;3A"
#define ALT_DOWN_ARROW "\x1b[1;3B"
#define ALT_RIGHT_ARROW "\x1b[1;3C"
#define ALT_LEFT_ARROW "\x1b[1;3D"


#define HOME_KEY "\x1b[H"
#define END_KEY "\x1b[F"
#define DELETE_KEY "\x1b[3~"
#define PAGE_UP "\x1b[5~"
#define PAGE_DOWN "\x1b[6~"
#define ESCAPE_KEY "\x1b"
#define ENTER_KEY "\r"
#define BACKSPACE_KEY "\x7f"
#define TAB_KEY "\t"

struct keymap_;

typedef void (*keymap_action)(const struct keymap_* keymap, void* userdata);

typedef struct keymap_ {
    char *seq;
    keymap_action action;
} keymap_t;



void onExit(const keymap_t* keymap, void* userdata);
void onEnter(const keymap_t* keymap, void* userdata);
void onLeftArrow(const struct keymap_ *keymap, void *userdata);
void onRightArrow(const struct keymap_ *keymap, void *userdata);
const keymap_t keymaps[] = {
    {"q", onExit},
    {"Q", onExit},
    {CTRL_C ,onExit},
    {ENTER_KEY ,onEnter},

    {UP_ARROW ,onRightArrow},
    {DOWN_ARROW ,onLeftArrow},
    {RIGHT_ARROW ,onRightArrow},
    {LEFT_ARROW ,onLeftArrow},

    {ALT_UP_ARROW ,onRightArrow},
    {ALT_DOWN_ARROW ,onLeftArrow},
    {ALT_RIGHT_ARROW ,onRightArrow},
    {ALT_LEFT_ARROW ,onLeftArrow},
};


int detect(char *seq, void *userdata) {
    if (seq == NULL || seq[0] == '\0') {
        return 0;  // Empty sequence has chance to match
    }

    int has_chance = 0;
    int full_match = 0;
    const char* matched_seq = NULL;

    // Check each known sequence
    int i = 0;
    for (i = 0; i < ARRAY_SIZE(keymaps); i++) {
        const char* key_seq = keymaps[i].seq;
        int seq_len = strlen(seq);
        int key_seq_len = strlen(key_seq);

        if (seq_len > key_seq_len) {
            continue;
        }
        assert(seq_len <= key_seq_len);

        if (strncmp(seq, key_seq, seq_len) == 0) {
            if (seq_len == key_seq_len) {
                keymap_action action = keymaps[i].action;
                if (action) {
                    action(&keymaps[i], userdata);
                }
                return 1;
            } else {
                return 0;
            }

        }
    }

    return -1;

}

// ====================================

typedef struct {
    // tty
    tty_t *tty;

    // path
    tokens_t parts;
    int highlight_idx;      // Index of currently highlighted part (0 to num_parts-1)

    // options
    bool do_exit :1;
    bool do_print :1;

    uint8_t exit_code;

} tui_state_t;



void tui_ttyinit(tui_state_t *state) {
    state->tty = malloc(sizeof(tty_t));
    tty_init(state->tty, "/dev/tty");
}

void tui_pathinit(tui_state_t *state, const char* filepath) {
    state->parts = split(filepath, "/");
    // default set to last one;
    state->highlight_idx = state->parts.count - 1;
}

void draw(tui_state_t *state) {
    tty_t *tty = state->tty;

    tty_setcol(tty, 0);
    tty_clearline(tty);

    // Build the path display with highlighting
    for (int i = 0; i < state->parts.count; i++) {

        char *delim;
        if (i != state->parts.count - 1) {
            delim = "/";
        } else {
            delim = "";
        }

        if (i == state->highlight_idx) {
            // Highlight this component
            tty_setinvert(tty);
            tty_printf(tty, "%s%s", state->parts.items[i], delim);
            tty_setnormal(tty);
        } else {
            // Normal display
            tty_printf(tty, "%s%s", state->parts.items[i], delim);
        }
    }

    // Add cursor indicator
    tty_setcol(tty, 0);

    // tty_moveup(tty, 1); // Move back to first line
    tty_flush(tty);
}

char keys[8] = {0};
int end = 0;
void handle_input(tui_state_t *state) {
    tty_t *tty = state->tty;

    int ret = tty_input_ready(tty, KEYTIMEOUT, 1);
    if (!ret) {
        end = 0;
        return;
    }
    keys[end++] = tty_getchar(tty);
    keys[end] = '\0';

    ret = detect(keys, state);
    if(ret) {
        end = 0;
        return;
    }
}

void onExit(const struct keymap_ *keymap, void *userdata) {
    tui_state_t *state = (tui_state_t *)userdata;
    state->do_exit = 1;
    state->exit_code = 1;
}

void onEnter(const struct keymap_ *keymap, void *userdata) {
    tui_state_t *state = (tui_state_t *)userdata;
    state->do_exit = 1;
    state->exit_code = 0;
    state->do_print = 1;
}

void onLeftArrow(const struct keymap_ *keymap, void *userdata) {
    tui_state_t *state = (tui_state_t *)userdata;
    if (state->highlight_idx > 0) {
        state->highlight_idx--;
    }
}

void onRightArrow(const struct keymap_ *keymap, void *userdata) {
    tui_state_t *state = (tui_state_t *)userdata;
    if (state->highlight_idx < state->parts.count - 1) {
        state->highlight_idx++;
    }
}

int tui_run(tui_state_t *state) {
    tty_t *tty = state->tty;

    while(1) {
        tty_getwinsz(tty);
        draw(state);
        handle_input(state);
        if (state->do_exit) {
            break;
        }
    }

    return state->exit_code;
}

void tui_ttycleanup(tui_state_t *state) {
    tty_setcol(state->tty, 0);
    tty_clearline(state->tty);
    tty_flush(state->tty);
    tty_reset(state->tty);
    free(state->tty);
}

void tui_pathcleanup(tui_state_t *state) {
    if (state->do_print) {
        for (int i = 0; i <= state->highlight_idx; i++) {
            printf("%s", state->parts.items[i]);
            if (i < state->highlight_idx) {
                printf("/");
            }
        }
        printf("\n");
    }
    nob_da_foreach(char *, it, &state->parts) {
        free(*it);
    }
    nob_da_free(state->parts);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filepath>\n", argv[0]);
        return 1;
    }

    tui_state_t state;
    memset(&state, 0, sizeof(state));

    tui_pathinit(&state, argv[1]);
    tui_ttyinit(&state);

    int exit_code = tui_run(&state);
    tui_ttycleanup(&state);
    tui_pathcleanup(&state);


    return exit_code;
}
