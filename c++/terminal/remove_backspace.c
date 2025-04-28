#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

void process_input(FILE *input) {
    int c;
    char buffer[BUFFER_SIZE];
    int pos = 0;
    
    while ((c = fgetc(input)) != EOF) {
        if (c == '\b') {
            // Handle backspace - move position back if possible
            if (pos > 0) {
                pos--;
            }
        } else {
            // Store character in buffer
            if (pos < BUFFER_SIZE - 1) {
                buffer[pos++] = c;
            } else {
                // Buffer full, flush it
                buffer[pos] = '\0';
                printf("%s", buffer);
                pos = 0;
                buffer[pos++] = c;
            }
        }
    }
    
    // Flush remaining characters in buffer
    if (pos > 0) {
        buffer[pos] = '\0';
        printf("%s", buffer);
    }
}

int main() {
    process_input(stdin);
    return 0;
}
