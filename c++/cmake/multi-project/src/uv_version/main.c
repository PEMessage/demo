#include <uv.h>
#include <stdio.h>

int main() {
    uv_loop_t *loop = uv_default_loop();

    uv_timer_t timer;
    uv_timer_init(loop, &timer);
    uv_timer_start(&timer, NULL, 0, 1000);

    uv_run(loop, UV_RUN_ONCE);

    printf("libuv version: %s\n", uv_version_string());

    uv_close((uv_handle_t *)&timer, NULL);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);

    return 0;
}
