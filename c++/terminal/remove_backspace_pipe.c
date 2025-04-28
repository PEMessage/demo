#include <stdio.h>
#include <unistd.h> // For read, write, STDIN_FILENO, STDOUT_FILENO
#include <stdlib.h> // For exit
#include <string.h> // Not strictly needed here, but often useful
#include <sys/select.h> // For select, fd_set, timeval
#include <errno.h> // For errno

#define BUFFER_SIZE 4096 // Size for the read buffer and the output buffer
#define TIMEOUT_SEC 1    // Timeout in seconds for flushing when idle
#define TIMEOUT_USEC 0   // Timeout in microseconds

// Buffer to hold processed output before flushing
char output_buffer[BUFFER_SIZE];
// Current number of characters in the output buffer
int output_buffer_len = 0;

/**
 * @brief Flushes the content of the output buffer to standard output.
 *
 * Handles potential partial writes by looping until the entire buffer is written.
 * Resets the buffer length after a successful write.
 * Prints errors to stderr if write fails.
 */
void flush_output_buffer() {
    // Only flush if there's something in the buffer
    if (output_buffer_len <= 0) {
        return;
    }

    ssize_t bytes_written_total = 0;
    // Loop to handle potential partial writes
    while (bytes_written_total < output_buffer_len) {
        ssize_t bytes_written = write(STDOUT_FILENO,
                                      output_buffer + bytes_written_total,
                                      output_buffer_len - bytes_written_total);

        if (bytes_written < 0) {
            // Check if the error was EINTR (Interrupted system call)
            if (errno == EINTR) {
                continue; // Retry the write operation
            }
            // For other errors, print message and discard buffer content
            perror("write error during flush");
            output_buffer_len = 0; // Clear buffer length to avoid infinite loop on error
            return; // Exit the function after error
        }
         if (bytes_written == 0) {
             // This typically means stdout was closed or is non-blocking and full
             fprintf(stderr, "Warning: write returned 0, stdout might be closed or full.\n");
             output_buffer_len = 0; // Discard buffer content
             return; // Exit the function
         }

        // Accumulate total bytes written
        bytes_written_total += bytes_written;
    }
    // Reset buffer length after successful flush
    output_buffer_len = 0;
}

int main() {
    fd_set readfds;         // Set of file descriptors for select()
    struct timeval timeout; // Timeout value for select()
    char read_buffer[BUFFER_SIZE]; // Temporary buffer for reading input chunks
    ssize_t bytes_read;     // Number of bytes read from stdin
    setbuf(stdout, NULL);

    // Main loop: continues until EOF is reached on stdin
    while (1) {
        // Initialize the file descriptor set for select()
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds); // Monitor standard input (file descriptor 0)

        // Reset the timeout structure for each select() call, as select() might modify it
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = TIMEOUT_USEC;

        // Wait for input on stdin or until the timeout expires
        // select() monitors file descriptors up to nfds-1, so we use STDIN_FILENO + 1
        int activity = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);

        // Check for errors during select() call (ignore EINTR)
        if (activity < 0 && errno != EINTR) {
            perror("select error");
            flush_output_buffer(); // Attempt to flush any remaining data before exiting
            return 1; // Exit with an error code
        }

        // Case 1: Timeout occurred (select returned 0)
        if (activity == 0) {
            // If a timeout happens and the output buffer has content, flush it
            flush_output_buffer();
            // Continue to the next iteration to wait for input again
            continue;
        }

        // Case 2: Input is available on stdin (select returned > 0)
        // We double-check if stdin is the one ready using FD_ISSET
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            // Read a chunk of data from standard input
            bytes_read = read(STDIN_FILENO, read_buffer, BUFFER_SIZE);

            // Check for read errors (ignoring EINTR)
            if (bytes_read < 0) {
                if (errno == EINTR) {
                    continue; // Interrupted by signal, just retry reading
                }
                perror("read error from stdin");
                flush_output_buffer(); // Attempt to flush before exiting
                return 1; // Exit with an error code
            }

            // Check for End-of-File (EOF) (read returned 0)
            if (bytes_read == 0) {
                flush_output_buffer(); // Flush any remaining content in the buffer
                break; // Exit the main loop gracefully
            }

            // Process the chunk of characters read from stdin
            for (int i = 0; i < bytes_read; ++i) {
                char current_char = read_buffer[i];

                // Handle Backspace ('\b')
                if (current_char == '\b') {
                    if (output_buffer_len > 0) {
                        // If buffer is not empty, remove the last character
                        output_buffer_len--;
                    } else {
                        // If buffer is empty, pass the backspace through directly.
                        // Flush any pending output first (though buffer is empty here)
                        // to maintain strict order if this logic changes.
                        flush_output_buffer();
                        // Write the backspace directly to stdout
                        ssize_t written = write(STDOUT_FILENO, &current_char, 1);
                        if (written <= 0) {
                             // Handle write error or EINTR for the passthrough backspace
                             if (written < 0 && errno == EINTR) {
                                 i--; // Need to retry processing this backspace character
                                 continue;
                             }
                             perror("write error (passthrough backspace)");
                             // Decide how to handle write error (e.g., exit)
                             return 1;
                        }
                    }
                }
                // Handle Newline ('\n')
                else if (current_char == '\n') {
                    // Check if the buffer is full before adding the newline
                    if (output_buffer_len >= BUFFER_SIZE) {
                        flush_output_buffer(); // Flush if full
                    }
                    // Add the newline character to the buffer
                    output_buffer[output_buffer_len++] = current_char;
                    // Flush the buffer because a newline was encountered
                    flush_output_buffer();
                }
                // Handle Normal Characters
                else {
                    // Check if the buffer is full before adding the character
                    if (output_buffer_len >= BUFFER_SIZE) {
                        flush_output_buffer(); // Flush if full
                    }
                    // Add the character to the output buffer
                    output_buffer[output_buffer_len++] = current_char;
                }
            } // End of processing loop for the read chunk
        } // End if (FD_ISSET)
    } // End while (1)

    // The final flush (on EOF) is handled inside the loop when read returns 0.
    return 0; // Indicate successful execution
}

