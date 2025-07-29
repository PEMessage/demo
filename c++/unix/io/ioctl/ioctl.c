#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#define IOCTL_CMD     TIOCGWINSZ
typedef struct winsize IOCTL_STRUCT;
#define IOCTL_FILE "/dev/tty"


void print_buffer(void *ptr, size_t len) {
    unsigned char *addr = (unsigned char *)ptr;

    // Print the pointer address in hex (byte by byte)
    printf("Content: \n");
    for (size_t i = 0; i < sizeof(void*); i++) {
        printf("%02X ", ((unsigned char*)&ptr)[i]);
        if (i % 8 == 8 -1) {
            printf("\n");
        }
    }
    printf("\n");
}

int main() {
    int fd;
    IOCTL_STRUCT buffer = {0};  // Zero-initialized

    // We use stdout (or any TTY) as the file descriptor for terminal ioctls
    printf("Using file: %s\n", IOCTL_FILE);
    fd =  open(IOCTL_FILE, O_RDONLY);

    printf("Using file descriptor: %d\n", fd);

    // Perform ioctl to get window size
    int ret = ioctl(fd, IOCTL_CMD, &buffer);
    printf("ioctl return value: %d\n", ret);

    if (ret < 0) {
        perror("ioctl IOCTL_CMD failed");
        return EXIT_FAILURE;
    }

    print_buffer(&buffer, sizeof(buffer));

    return EXIT_SUCCESS;
}
