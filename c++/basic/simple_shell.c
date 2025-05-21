#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_INPUT_LEN 100

// Predefined command functions
void cmd_help();
void cmd_echo(char *args);
void cmd_add(char *args);
void cmd_exit();

int main() {
    char input[MAX_INPUT_LEN];
    char cmd[MAX_INPUT_LEN];
    char args[MAX_INPUT_LEN];
    int i, ch;
    
    printf("Simple Shell - Type 'help' for available commands\n");
    
    while (1) {
        // Print prompt
        printf("> ");
        
        // Read input
        i = 0;
        while ((ch = getchar()) != '\n' && ch != EOF && i < MAX_INPUT_LEN - 1) {
            input[i++] = ch;
        }
        input[i] = '\0';
        
        // Skip empty input
        if (i == 0) continue;
        
        // Parse command and arguments
        int cmd_end = 0;
        while (cmd_end < i && input[cmd_end] != ' ') {
            cmd_end++;
        }
        
        strncpy(cmd, input, cmd_end);
        cmd[cmd_end] = '\0';
        
        if (cmd_end < i) {
            strcpy(args, input + cmd_end + 1);
        } else {
            args[0] = '\0';
        }
        
        // Execute command
        if (strcmp(cmd, "help") == 0) {
            cmd_help();
        } else if (strcmp(cmd, "echo") == 0) {
            cmd_echo(args);
        } else if (strcmp(cmd, "add") == 0) {
            cmd_add(args);
        } else if (strcmp(cmd, "exit") == 0) {
            cmd_exit();
        } else {
            printf("Unknown command: %s\n", cmd);
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
