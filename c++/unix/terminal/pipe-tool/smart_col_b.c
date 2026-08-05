#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>

#define MAX 4096

static char out[MAX];   /* pending output; backspace can undo it */
static int n;

static void flush(void) {
    if (n) {
        fwrite(out, 1, n, stdout);
        fflush(stdout);
        n = 0;
    }
}

/* read with timeout (fd must be open, timeout_sec seconds):
   >0 bytes read; 0 EOF; -1 error, timeout sets errno == ETIMEDOUT */
static ssize_t read_timeout(int fd, char *buf, size_t size, int timeout_sec) {
    struct timeval tv = { .tv_sec = timeout_sec };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    int r = select(fd + 1, &fds, NULL, NULL, &tv);
    if (r < 0) return -1;                     /* EINTR etc., up to the caller */
    if (r == 0) { errno = ETIMEDOUT; return -1; } /* timed out */

    return read(fd, buf, size);               /* fd is readable, won't block */
}

/* Return the next input character; -1 on EOF */
static int next_char(void) {
    static char in[MAX];
    static ssize_t pos, len;
    for (;;) {
        if (pos < len) return (unsigned char)in[pos++];

        len = read_timeout(STDIN_FILENO, in, sizeof in, 1);
        if (len > 0) { pos = 0; continue; }
        if (len == 0) return EOF;

        switch (errno) {
        case EINTR:    continue;          /* interrupted by a signal, retry */
        case ETIMEDOUT: flush(); continue; /* idle timeout: send out what we have */
        default:       perror("read"); exit(1);
        }
    }
}

int main(void) {
    int c;
    while ((c = next_char()) != EOF) {
        if (c == '\b') {
            if (n) n--;                  /* undo the last buffered character */
            else {                       /* nothing to undo, pass through */
                fputc('\b', stdout);
                fflush(stdout);
            }
        } else {
            if (n == MAX) flush();
            out[n++] = c;
            if (c == '\n') flush();      /* flush immediately on newline */
        }
    }
    flush();
    return 0;
}
