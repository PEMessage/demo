#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h> // Required for errno

int main() {
    int pfd[2]; // Pipe file descriptors: pfd[0] is read end, pfd[1] is write end
    pid_t pid;
    char buf[100];
    int bytes_available;
    ssize_t bytes_read;
    const char *message = "Hello from parent!";

    // Create the pipe
    if (pipe(pfd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    // Fork a child process
    pid = fork();

    if (pid == -1) {
        perror("fork failed");
        // Clean up pipe before exiting
        close(pfd[0]);
        close(pfd[1]);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // --- Child Process (Reader) ---
        close(pfd[1]); // Close unused write end

        printf("[Child] Waiting for data...\n");
        // Optional: Wait a moment for the parent to write
        sleep(1);

        // Check how many bytes are available using ioctl FIONREAD
        if (ioctl(pfd[0], FIONREAD, &bytes_available) == -1) {
            perror("[Child] ioctl FIONREAD failed");
        } else {
            printf("[Child] ioctl(FIONREAD) reports %d bytes available before reading.\n", bytes_available);
        }

        // Try reading some data (less than the full message to show consumption)
        printf("[Child] Attempting to read up to 10 bytes...\n");
        bytes_read = read(pfd[0], buf, 10); // Read max 10 bytes

        if (bytes_read == -1) {
            perror("[Child] read failed");
        } else if (bytes_read == 0) {
             printf("[Child] Pipe closed (EOF).\n");
        } else {
            buf[bytes_read] = '\0'; // Null-terminate the string
            printf("[Child] Read %zd bytes: \"%s\"\n", bytes_read, buf);

            // Check available bytes again AFTER reading
            if (ioctl(pfd[0], FIONREAD, &bytes_available) == -1) {
                perror("[Child] ioctl FIONREAD failed after read");
            } else {
                printf("[Child] ioctl(FIONREAD) reports %d bytes available AFTER reading.\n", bytes_available);
            }

            // Read the rest (if any) - uncomment to see it empty the pipe
            /*
            printf("[Child] Reading remaining data...\n");
            bytes_read = read(pfd[0], buf, sizeof(buf)-1);
            if (bytes_read > 0) {
                 buf[bytes_read] = '\0';
                 printf("[Child] Read remaining %zd bytes: \"%s\"\n", bytes_read, buf);
                 if (ioctl(pfd[0], FIONREAD, &bytes_available) != -1) {
                     printf("[Child] ioctl(FIONREAD) reports %d bytes available finally.\n", bytes_available);
                 }
            } else if (bytes_read == 0) {
                 printf("[Child] No more data (EOF).\n");
            } else {
                 perror("[Child] read failed");
            }
            */
        }

        close(pfd[0]); // Close read end
        printf("[Child] Exiting.\n");
        exit(EXIT_SUCCESS);

    } else {
        // --- Parent Process (Writer) ---
        close(pfd[0]); // Close unused read end

        printf("[Parent] Writing message: \"%s\"\n", message);
        ssize_t bytes_written = write(pfd[1], message, strlen(message));

        if (bytes_written == -1) {
            perror("[Parent] write failed");
            // Consider signaling child or handling error more robustly
        } else {
             printf("[Parent] Wrote %zd bytes.\n", bytes_written);
        }


        close(pfd[1]); // Close write end (sends EOF to reader when buffer empty)
        printf("[Parent] Waiting for child to finish...\n");
        wait(NULL); // Wait for child process to terminate
        printf("[Parent] Child finished. Exiting.\n");
        exit(EXIT_SUCCESS);
    }

    return 0; // Should not be reached
}

