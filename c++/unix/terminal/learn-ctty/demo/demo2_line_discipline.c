/*
 * L1: Line discipline (n_tty).
 *
 * The watershed between a tty and a plain file: read/write go through a
 * line discipline that adds "lines" and "key semantics". cfmakeraw()
 * turns all of it off (and OPOST/ONLCR: with those off, output must use
 * '\r\n', not '\n').
 *
 * Phase A (canonical): read() returns a whole line at once; echo on;
 * ^C becomes SIGINT. Phase B (raw): per-byte, no echo; ^C is 0x03.
 *
 * Run: ./demo2_line_discipline
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>
#include <signal.h>

static volatile sig_atomic_t sigint_seen;

static void on_sigint(int s)
{
	(void)s;
	sigint_seen = 1;
}

static void print_bytes(const unsigned char *p, ssize_t n)
{
	for (ssize_t i = 0; i < n; i++)
		printf("  byte 0x%02x (%c)\n", p[i], (p[i] >= 32 && p[i] < 127) ? p[i] : ' ');
}

int main(void)
{
	struct termios orig, raw;
	struct sigaction sa = { 0 };
	char buf[128];

	printf("line-discipline flags:\n");
	printf("  ICANON  line buffering + editing (a mini readline)\n");
	printf("  ECHO    echo what you type\n");
	printf("  ISIG    Ctrl+C->SIGINT, Ctrl+Z->SIGTSTP\n");
	printf("  OPOST   output processing, '\\n' -> '\\r\\n'\n");

	tcgetattr(0, &orig);
	sa.sa_handler = on_sigint;         /* no SA_RESTART: read() returns EINTR */
	sigaction(SIGINT, &sa, NULL);

	/* ---------- phase A: canonical ---------- */
	printf("\nA: canonical. type a line then Enter (^C finishes this phase):\n");
	for (;;) {
		fflush(stdout);
		ssize_t n = read(0, buf, sizeof(buf));
		if (n < 0 && errno == EINTR) {
			printf("A: ^C became SIGINT (ISIG on)\n");
			break;
		}
		printf("A: read() returned %zd bytes at once:\n", n);
		print_bytes((const unsigned char *)buf, n);
	}

	/* ---------- phase B: raw ---------- */
	tcgetattr(0, &raw);
	cfmakeraw(&raw);
	tcsetattr(0, TCSANOW, &raw);
	tcflush(0, TCIFLUSH);
	printf("\r\nB: raw mode. type 3 keys then ^C (just a byte now):\r\n");

	unsigned char c;
	int got = 0;
	while (read(0, &c, 1) == 1) {
		if (c == 0x03) {
			printf("\r\nB: ^C is byte 0x03, not a signal (ISIG off)\r\n");
			break;
		}
		printf("  byte 0x%02x\r\n", c);
		if (++got == 3)
			printf("     per-byte, no echo\r\n");
	}

	tcsetattr(0, TCSANOW, &orig);
	printf("restored, bye\n");
	return 0;
}
