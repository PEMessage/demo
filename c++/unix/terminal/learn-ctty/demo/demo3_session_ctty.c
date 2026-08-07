/*
 * L2: session + controlling terminal
 *
 * rule: only a session leader can acquire a controlling terminal
 *   - new_session() (setsid) makes the caller a session leader
 *     (sid == pid)
 *   - a session leader opening an unused tty gets it as its ctty
 *     automatically, or explicitly via acquire_ctty() (TIOCSCTTY)
 *   - the ctty is where keyboard signals go and what /dev/tty points to
 *
 * Single file, no dependencies. Stage-by-stage:
 *   [1] parent, in the shell's session
 *   [2] child tries acquire_ctty() before new_session() -> EPERM
 *   [3] child calls new_session()
 *   [4] child acquire_ctty() -> ok, pty becomes its ctty; prove it works
 *
 * Run: ./demo3_session_ctty
 */
#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

/* ---- primitives ---- */

/* start a new session; returns the new sid, or -1 on error */
static int new_session(void)
{
	return setsid();
}

/* 1 if this process is a session leader (sid == pid), 0 otherwise */
static int is_session_leader(void)
{
	return getpid() == getsid(0);
}

/* does this process have a controlling terminal? */
static int has_ctty(void)
{
	return tcgetpgrp(STDIN_FILENO) >= 0;
}

/* open a fresh pty (master+slave); returns 0 on success, -1 on error */
static int pty_pair_open(int *master, int *slave)
{
	int m = posix_openpt(O_RDWR | O_NOCTTY);
	if (m < 0)
		return -1;
	if (grantpt(m) != 0 || unlockpt(m) != 0) {
		close(m);
		return -1;
	}
	*master = m;
	*slave = open(ptsname(m), O_RDWR | O_NOCTTY);
	if (*slave < 0) {
		close(m);
		return -1;
	}
	return 0;
}

/* make fd the controlling terminal of this session; 0 on success, -1 on error */
static int acquire_ctty(int fd)
{
	return ioctl(fd, TIOCSCTTY, 0);
}

/* write msg on slave, read it back from master; returns bytes, -1 on error */
static ssize_t pty_roundtrip(int master, int slave, const char *msg,
			     char *buf, size_t bufsz)
{
	size_t len = strlen(msg);
	if (write(slave, msg, len) != (ssize_t)len)
		return -1;
	ssize_t n = read(master, buf, bufsz);
	if (n < 0)
		return -1;
	while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
		n--;                    /* strip the \r\n added by ONLCR */
	return n;
}

/* one aligned state line: pid / sid / session-leader? / ctty? */
static void state(const char *indent, const char *label)
{
	printf("%s%-16s pid=%-6d sid=%-6d leader=%-3s ctty=%s\n",
	       indent, label, getpid(), getsid(0),
	       is_session_leader() ? "yes" : "no",
	       has_ctty() ? "yes" : "no");
}

int main(void)
{
	printf("L2: session + controlling terminal\n");
	printf("rule: only a session leader can get a ctty (sid == pid)\n\n");

	printf("[1] parent, in the shell's session\n");
	state("      ", "parent");

	pid_t pid = fork();
	if (pid == 0) {
		int master, slave;
		if (pty_pair_open(&master, &slave) < 0) {
			perror("pty_pair_open");
			_exit(1);
		}

		printf("\n[2] child tries acquire_ctty() before new_session()\n");
		state("      ", "child");
		errno = 0;
		if (acquire_ctty(slave) < 0)
			printf("      acquire_ctty() -> %s\n", strerror(errno));
		printf("      (rejected: not a session leader yet)\n");

		printf("\n[3] child calls new_session()\n");
		new_session();
		state("      ", "child");
		printf("      (sid == pid -> now a session leader)\n");

		printf("\n[4] child calls acquire_ctty(slave)\n");
		errno = 0;
		if (acquire_ctty(slave) < 0)
			perror("      acquire_ctty()");
		else
			printf("      acquire_ctty() -> ok, pty is now the child's ctty\n");

		printf("      tcgetpgrp(slave) = %d\n", tcgetpgrp(slave));

		/* prove the ctty is live: write on slave, read back on master */
		char b[16];
		ssize_t n = pty_roundtrip(master, slave, "PING", b, sizeof(b));
		printf("      roundtrip: wrote \"PING\" on slave, master read \"%.*s\"\n",
		       (int)n, b);
		_exit(0);
	}

	waitpid(pid, NULL, 0);
	return 0;
}
