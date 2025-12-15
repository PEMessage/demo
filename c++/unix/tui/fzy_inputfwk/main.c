#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tty.h"
#include "config.h"

// ====================================
#define CR_FIELD \
    int line

typedef struct {
    CR_FIELD;
} cr_ctx_t;

#define CR_START(ctx)     switch((ctx)->line) { case 0:
#define CR_YIELD(ctx)     do { (ctx)->line = __LINE__; return; case __LINE__:; } while(0)
#define CR_RESET(ctx)     do { (ctx)->line = 0; } while(0)
#define CR_END(ctx)       } (ctx)->line = 0;

/* Helpers for timing and waiting */
#define CR_AWAIT(ctx, cond) \
    while(!(cond)) { CR_YIELD(ctx); }

#define CR_IF_RESET(ctx, cond) \
    if(cond) { CR_RESET(ctx); return; }


typedef struct {
    CR_FIELD;
} cr_input_t;
// ====================================

typedef struct {
    tty_t *tty;
    int code;
    int exit;
} tui_state_t;

void tui_init(tui_state_t *state) {
    state->tty = malloc(sizeof(tty_t));
    tty_init(state->tty, "/dev/tty");
}

void draw(tui_state_t *state) {
    tty_t *tty = state->tty;

    tty_setcol(tty, 0);
    tty_printf(tty, "Hello World %d", state->code);
    tty_clearline(tty);
    tty_printf(tty, "\n");

    tty_moveup(tty, 1); // this is key
    tty_flush(tty);
}

#define CTRL_C 3
#define ESC 27
#define UP_ARROW 'A'
#define DOWN_ARROW 'B'
#define RIGHT_ARROW 'C'
#define LEFT_ARROW 'D'

void handle_input(cr_input_t *ctx, tui_state_t *state) {
    tty_t *tty = state->tty;
    char key = 0;
    int res = 0;

    CR_START(ctx);
    while(1) {
        // First key
        CR_IF_RESET(ctx, tty_input_ready(tty, KEYTIMEOUT, 1) != 1);
        key = tty_getchar(tty);
        if (key == 'q' || key == 'Q' || key == CTRL_C) {
            state->exit = 1;
            CR_RESET(ctx);
            return;
        } else if (key = ESC) {
            // do nothing, keep check
        } else {
            CR_RESET(ctx);
            return;
        }
        CR_YIELD(ctx);

        // Second Key
        CR_IF_RESET(ctx, tty_input_ready(tty, KEYTIMEOUT, 1) != 1);
        key = tty_getchar(tty);
        if (key != '[') {
            CR_RESET(ctx);
            return;
        }
        CR_YIELD(ctx);


        // Third Key
        CR_IF_RESET(ctx, tty_input_ready(tty, KEYTIMEOUT, 1) != 1);
        key = tty_getchar(tty);
        if (key == UP_ARROW || key == DOWN_ARROW || key == LEFT_ARROW || key == RIGHT_ARROW) {
            state->code = key;
        }
        CR_RESET(ctx);
        return;

    }
    CR_END(ctx);
}

int tui_run(tui_state_t *state) {
    tty_t *tty = state->tty;

    cr_input_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    while(1) {
        tty_getwinsz(tty);
        draw(state);
        handle_input(&ctx, state);
        if (state->exit) {
            break;
        }
    }

    return 0;
}

void tui_cleanup(tui_state_t *state) {
    tty_reset(state->tty);
    free(state->tty);
}

int main(void) {
    tui_state_t state;
    memset(&state, 0, sizeof(state));

    tui_init(&state);
    int exit_code = tui_run(&state);
    tui_cleanup(&state);

    return exit_code;
}
