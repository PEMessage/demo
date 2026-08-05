/*
 * L0: A terminal is just a readable/writable file.
 *
 * Minimal semantics: a tty is only a char device in the kernel.
 * open() gives you an fd, write() sends bytes, read() receives bytes.
 * No different from a regular file or pipe. At this layer the kernel
 * does not need to know it is a terminal at all.
 *
 * Run: ./demo1_tty_file   (needs a real terminal / pty)
 */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(void)
{
	/* Open the current terminal as if it were a plain file */
	int fd = open("/dev/tty", O_RDWR | O_NOCTTY);
	if (fd < 0) {
		perror("open /dev/tty");
		return 1;
	}

	/* write(): bytes written into the terminal show up on screen */
	write(fd, "L0: tty is just a readable/writable file\n",
	      strlen("L0: tty is just a readable/writable file\n"));
	write(fd, "write() sends: ", strlen("write() sends: "));
	write(fd, "hello, tty!\n", strlen("hello, tty!\n"));

	/* read(): read bytes from the terminal like from a file */
	write(fd, "read() blocks: type a line and press Enter\n",
	      strlen("read() blocks: type a line and press Enter\n"));
	char buf[256];
	ssize_t n = read(fd, buf, sizeof(buf));
	if (n > 0) {
		write(fd, "received: ", strlen("received: "));
		write(fd, buf, n);
	}

	close(fd);
	return 0;
}
