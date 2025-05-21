#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_INPUT_LEN 100
#define MAX_TOKENS 32

// Predefined command functions
void cmd_help();
void cmd_echo(char *args);
void cmd_add(char *args);
void cmd_exit();

int main() {
    char input[MAX_INPUT_LEN];
    char *tokens[MAX_TOKENS];
    char *saveptr;
    int i, ch;
    
    printf("Simple Shell - Type 'help' for available commands\n");
    
    while (1) {
        // 1.0 -- Print prompt
        printf("> ");
        fflush(stdout);  // Ensure prompt is displayed immediately
        
        // 1.1 -- Read input
        i = 0;
        while ((ch = getchar()) != '\n' && ch != EOF && i < MAX_INPUT_LEN - 1) {
            input[i++] = ch;
        }
        
        // 1.2 -- Handle EOF (Ctrl+D)
        if ((ch == EOF) && (i == 0)) {
            // EOF at start of line - treat as exit command
            printf("\n");
            cmd_exit();
        } 
        
        // 1.3 -- Skip empty input
        if (i == 0) continue;
        input[i] = '\0';
        
        // 2.0 -- Tokenize input using strtok_r(now we have input as cstring)
        char *token;
        int token_count = 0;

        token = strtok_r(input, " ", &saveptr);
        while (token != NULL && token_count < MAX_TOKENS - 1) {
            tokens[token_count++] = token;
            token = strtok_r(NULL, " ", &saveptr);
        }
        tokens[token_count] = NULL;  // Null-terminate the token array

        if (token_count == 0) continue;  // Shouldn't happen due to earlier check
                                         // Update: without this, pure space input will cause error

        // Execute command
        if (strcmp(tokens[0], "help") == 0) {
            cmd_help();
        } else if (strcmp(tokens[0], "echo") == 0) {
            // Combine remaining tokens into args string
            char args[MAX_INPUT_LEN] = {0};
            for (int j = 1; j < token_count; j++) {
                if (j > 1) strcat(args, " ");
                strcat(args, tokens[j]);
            }
            cmd_echo(args);
        } else if (strcmp(tokens[0], "add") == 0) {
            // Combine remaining tokens into args string
            char args[MAX_INPUT_LEN] = {0};
            for (int j = 1; j < token_count; j++) {
                if (j > 1) strcat(args, " ");
                strcat(args, tokens[j]);
            }
            cmd_add(args);
        } else if (strcmp(tokens[0], "exit") == 0) {
            cmd_exit();
        } else {
            printf("Unknown command: %s\n", tokens[0]);
        }
    }
    
    return 0;
}

void cmd_help() {
    printf("Available commands:\n");
    printf("  help          - Show this help message\n");
    printf("  echo <text>   - Echo back the provided text\n");
    printf("  add <num1> <num2> - Add two numbers\n");
    printf("  exit          - Exit the shell\n");
}

void cmd_echo(char *args) {
    if (strlen(args) == 0) {
        printf("Usage: echo <text>\n");
    } else {
        printf("%s\n", args);
    }
}

void cmd_add(char *args) {
    if (strlen(args) == 0) {
        printf("Usage: add <num1> <num2>\n");
        return;
    }
    
    double num1, num2;
    int parsed = sscanf(args, "%lf %lf", &num1, &num2);
    
    if (parsed != 2) {
        printf("Error: Please provide two numbers\n");
    } else {
        printf("Result: %.2f\n", num1 + num2);
    }
}

void cmd_exit() {
    printf("Exiting shell...\n");
    exit(0);
}
