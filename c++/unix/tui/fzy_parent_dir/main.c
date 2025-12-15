#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include "tty.h"
#include "config.h"


#define ARRAY_SIZE(a) (sizeof((a)) / sizeof((a[0])))
#define ARRAY_INDEX(ptr, array) ((size_t)((void *)(ptr) - (void *)(array)) / sizeof((array)[0]))
// ====================================
// Key sequence definitions
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
void onArrow(const struct keymap_ *keymap, void *userdata);

const keymap_t keymaps[] = {
    {"q", onExit},
    {"Q", onExit},
    {CTRL_C ,onExit},
    {ENTER_KEY ,onEnter},

    {UP_ARROW ,onArrow},
    {DOWN_ARROW ,onArrow},
    {RIGHT_ARROW ,onArrow},
    {LEFT_ARROW ,onArrow},

    {ALT_UP_ARROW ,onArrow},
    {ALT_DOWN_ARROW ,onArrow},
    {ALT_RIGHT_ARROW ,onArrow},
    {ALT_LEFT_ARROW ,onArrow},
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
    tty_t *tty;
    char *filepath;          // The complete file path
    char **parts;           // Array of path components
    int num_parts;          // Number of path components
    int highlight_idx;      // Index of currently highlighted part (0 to num_parts-1)

    bool do_exit :1;
    bool do_print :1;
} tui_state_t;

// Function to split a path into components
char** split_path(const char* path, int* num_parts) {
    if (!path || !num_parts) return NULL;

    // Make a copy to work with
    char* path_copy = strdup(path);
    if (!path_copy) return NULL;

    // Count the number of components
    int count = 0;
    char* token = strtok(path_copy, "/");
    while (token) {
        count++;
        token = strtok(NULL, "/");
    }

    free(path_copy);

    // Allocate array for parts
    char** parts = malloc(count * sizeof(char*));
    if (!parts) return NULL;

    // Split again and store
    path_copy = strdup(path);
    if (!path_copy) {
        free(parts);
        return NULL;
    }

    int i = 0;
    token = strtok(path_copy, "/");
    while (token && i < count) {
        parts[i] = strdup(token);
        if (!parts[i]) {
            // Cleanup on failure
            for (int j = 0; j < i; j++) free(parts[j]);
            free(parts);
            free(path_copy);
            return NULL;
        }
        i++;
        token = strtok(NULL, "/");
    }

    free(path_copy);
    *num_parts = count;
    return parts;
}

// Function to free path components
void free_path_parts(char** parts, int num_parts) {
    if (!parts) return;
    for (int i = 0; i < num_parts; i++) {
        free(parts[i]);
    }
    free(parts);
}

void tui_init(tui_state_t *state, const char* filepath) {
    state->tty = malloc(sizeof(tty_t));
    tty_init(state->tty, "/dev/tty");

    // Store filepath
    state->filepath = strdup(filepath);

    // Split path into components
    state->parts = split_path(filepath, &state->num_parts);

    // Start with last component highlighted (like fzy_inputfwk)
    state->highlight_idx = state->num_parts - 1;
}

void draw(tui_state_t *state) {
    tty_t *tty = state->tty;

    tty_setcol(tty, 0);
    tty_clearline(tty);

    // Build the path display with highlighting
    for (int i = 0; i < state->num_parts; i++) {
        if (i > 0) {
            tty_setnormal(tty);
            tty_printf(tty, "/");
        }

        if (i == state->highlight_idx) {
            // Highlight this component
            tty_setinvert(tty);
            tty_printf(tty, "%s", state->parts[i]);
            tty_setnormal(tty);
        } else {
            // Normal display
            tty_printf(tty, "%s", state->parts[i]);
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
}

void onEnter(const struct keymap_ *keymap, void *userdata) {
    tui_state_t *state = (tui_state_t *)userdata;
    state->do_exit = 1;
    state->do_print = 1;
}

void onArrow(const struct keymap_ *keymap, void *userdata) {
    tui_state_t *state = (tui_state_t *)userdata;

    // Check which arrow key was pressed
    const char* seq = keymap->seq;

    if (strcmp(seq, LEFT_ARROW) == 0 || strcmp(seq, ALT_LEFT_ARROW) == 0) {
        // Move highlight left
        if (state->highlight_idx > 0) {
            state->highlight_idx--;
        }
    } else if (strcmp(seq, RIGHT_ARROW) == 0 || strcmp(seq, ALT_RIGHT_ARROW) == 0) {
        // Move highlight right
        if (state->highlight_idx < state->num_parts - 1) {
            state->highlight_idx++;
        }
    }
    // Up/Down arrows don't do anything for this demo
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

    return 0;
}

void tui_cleanup(tui_state_t *state) {
    tty_setcol(state->tty, 0);
    tty_clearline(state->tty);
    tty_flush(state->tty);
    tty_reset(state->tty);
    free(state->tty);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filepath>\n", argv[0]);
        return 1;
    }

    tui_state_t state;
    memset(&state, 0, sizeof(state));

    tui_init(&state, argv[1]);
    int exit_code = tui_run(&state);

    tui_cleanup(&state);

    if (state.do_print) {
        // Print path up to (but not including) the highlighted component
        printf("/");  // Start with root
        for (int i = 0; i <= state.highlight_idx; i++) {
            printf("%s", state.parts[i]);
            if (i < state.highlight_idx) {
                printf("/");
            }
        }
        printf("\n");
    }
    free(state.filepath);
    free_path_parts(state.parts, state.num_parts);

    return exit_code;
}
