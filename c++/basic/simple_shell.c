#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_INPUT_LEN 100
#define MAX_TOKENS 32
#define MAX_COMMANDS 32

typedef struct {
    const char *name;
    void (*func)(int argc, char **argv);
    const char *description;
} cmd;

// Global command list

// Predefined command functions
void cmd_help(int argc, char **argv);
void cmd_echo(int argc, char **argv);
void cmd_add(int argc, char **argv);
void cmd_exit(int argc, char **argv);
void cmd_loop(int argc, char **argv);

const cmd command_list[] = {
    {"help", cmd_help, "Show this help message"},
    {"echo", cmd_echo, "Echo back the provided text"},
    {"add", cmd_add, "Add two numbers"},
    {"exit", cmd_exit, "Exit the shell"},
    {"loop", cmd_loop, "Loop n times cmd"}
};
int command_count = sizeof(command_list) / sizeof(cmd);


const cmd* find_command(const char *name) {
    for (int i = 0; i < command_count; i++) {
        if (strcmp(command_list[i].name, name) == 0) {
            return &command_list[i];
        }
    }
    return NULL;
}



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
            cmd_exit(0, NULL);
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
        const cmd *command = find_command(tokens[0]);
        if (command != NULL) {
            command->func(token_count - 1, tokens + 1);
        } else {
            printf("Unknown command: %s\n", tokens[0]);
        }
    }
    
    return 0;
}

void cmd_help(int argc, char **argv) {
    printf("Available commands:\n");
    for (int i = 0; i < command_count; i++) {
        printf("  %-10s - %s\n", command_list[i].name, command_list[i].description);
    }
}

void cmd_echo(int argc, char **argv) {
    if (argc == 0) {
        printf("Usage: echo <text>\n");
        return;
    }

    for (int i = 0; i < argc; i++) {
        printf("%s%s", argv[i], (i < argc - 1) ? " " : "");
    }
    printf("\n");
}

void cmd_add(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: add <num1> <num2>\n");
        return;
    }

    double num1, num2;
    if (sscanf(argv[0], "%lf", &num1) != 1 || sscanf(argv[1], "%lf", &num2) != 1) {
        printf("Error: Please provide two valid numbers\n");
        return;
    }

    printf("Result: %.2f\n", num1 + num2);
}

void cmd_loop(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: loop N CMD ARGS\n");
        return;
    }

    int i = 0;
    int times = 0;
    if ( sscanf(argv[0], "%d", &times) != 1 ) {
        printf("Error: Please provide a valid numbers\n");
    }
    argv++;
    argc--;

    const cmd *command = find_command(argv[0]);
    if (command == NULL) {
        printf("Unknown command: %s\n", argv[0]);
        return ;
    } 
    argv++;
    argc--;

    for ( i = 0 ; i < times ; i ++ ) {
        command->func(argc, argv);
    }
}

void cmd_exit(int argc, char **argv) {
    printf("Exiting shell...\n");
    exit(0);
}
