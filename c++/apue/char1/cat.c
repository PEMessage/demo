
#include <unistd.h> // for STDIN_FILENO STDOUT_FILENO
#include <stdlib.h> // for exit
#define BUFFSIZE 4096
#define WRITE_ERR_MSG "Err: Write error\n"
#define READ_ERR_MSG "Err: Read error\n"

int main(void)
{
    int n;
    char buf[BUFFSIZE];

    while ((n = read(STDIN_FILENO, buf, BUFFSIZE)) > 0) {
        if (write(STDOUT_FILENO, buf, n) != n) {
            write(STDERR_FILENO, WRITE_ERR_MSG, sizeof(WRITE_ERR_MSG));
            exit(1);
        }
    }

    if (n < 0) {
        write(STDERR_FILENO, READ_ERR_MSG, sizeof(READ_ERR_MSG));
        exit(1);
    }

    exit(0);
}
