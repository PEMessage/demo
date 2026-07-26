#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 7000
#define BACKLOG 128
#define BUFSIZE 4096

// ─── uv (non-blocking) ───────────────────────────────────────────────

typedef struct {
    uv_tcp_t stream;
} client_t;

static uv_loop_t *loop;

static void on_client_closed(uv_handle_t *handle) {
    free((client_t *)handle);
}

static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

static void on_data(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    if (nread <= 0) {
        free(buf->base);
        uv_close((uv_handle_t *)stream, on_client_closed);
        return;
    }

    uv_write_t *req = malloc(sizeof(uv_write_t));
    uv_buf_t wbuf = uv_buf_init(buf->base, nread);
    uv_write(req, stream, &wbuf, 1, NULL);
    free(buf->base);
}

static void on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) return;

    client_t *client = malloc(sizeof(client_t));
    uv_tcp_init(loop, &client->stream);

    if (uv_accept(server, (uv_stream_t *)&client->stream) == 0) {
        uv_read_start((uv_stream_t *)&client->stream, alloc_buffer, on_data);
    } else {
        uv_close((uv_handle_t *)&client->stream, on_client_closed);
    }
}

static int run_uv(void) {
    loop = uv_default_loop();

    uv_tcp_t server;
    uv_tcp_init(loop, &server);

    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", PORT, &addr);

    uv_tcp_bind(&server, (const struct sockaddr *)&addr, 0);
    int r = uv_listen((uv_stream_t *)&server, BACKLOG, on_new_connection);
    if (r) {
        fprintf(stderr, "listen error: %s\n", uv_strerror(r));
        return 1;
    }

    printf("[uv] echo server on :%d\n", PORT);
    return uv_run(loop, UV_RUN_DEFAULT);
}

// ─── blocking ────────────────────────────────────────────────────────

static int run_blocking(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }
    if (listen(server_fd, BACKLOG) < 0) { perror("listen"); return 1; }

    printf("[blocking] echo server on :%d\n", PORT);

    for (;;) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
        if (client_fd < 0) { perror("accept"); continue; }

        char buf[BUFSIZE];
        ssize_t n;
        while ((n = read(client_fd, buf, sizeof(buf))) > 0) {
            write(client_fd, buf, n);
        }
        close(client_fd);
    }
}

// ─── main ────────────────────────────────────────────────────────────

int main(int argc, char **argv) {
    const char *mode = (argc > 1) ? argv[1] : "uv";

    if (strcmp(mode, "blocking") == 0) {
        return run_blocking();
    }
    if (strcmp(mode, "uv") == 0) {
        return run_uv();
    }

    fprintf(stderr, "usage: %s [uv|blocking]\n", argv[0]);
    return 1;
}
