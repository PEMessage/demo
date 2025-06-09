#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source> <destination>\n", argv[0]);
        return 1;
    }

    const char *source = argv[1];
    const char *dest = argv[2];

    // Open source file
    int source_fd = open(source, O_RDONLY);
    if (source_fd == -1) {
        perror("open source");
        return 1;
    }

    // Get file stats
    struct stat stat_buf;
    if (fstat(source_fd, &stat_buf) == -1) {
        perror("fstat");
        close(source_fd);
        return 1;
    }

    // Open destination file
    int dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, stat_buf.st_mode);
    if (dest_fd == -1) {
        perror("open destination");
        close(source_fd);
        return 1;
    }

    // Copy using sendfile
    off_t offset = 0;
    ssize_t result = sendfile(dest_fd, source_fd, &offset, stat_buf.st_size);
    if (result == -1) {
        perror("sendfile");
        close(source_fd);
        close(dest_fd);
        return 1;
    }

    // Close files
    close(source_fd);
    close(dest_fd);

    return 0;
}
