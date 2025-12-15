#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tty.h"


typedef struct {
    tty_t *tty;
} tui_state_t;

void tui_init(tui_state_t *state) {
    state->tty = malloc(sizeof(tty_t));
    tty_init(state->tty, "/dev/tty");
}


int tui_run(tui_state_t *state) {
    tty_t *tty = state->tty;
    tty_setcol(tty, 0);
    tty_printf(tty, "> Hello World \n");

    return 0;
}

void tui_cleanup(tui_state_t *state) {
    tty_reset(state->tty);
    free(state->tty);
}

int main(void) {
    tui_state_t state;

    tui_init(&state);
    int exit_code = tui_run(&state);
    tui_cleanup(&state);

    return exit_code;
}
