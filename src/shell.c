#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "parser.h"
#include "executor.h"
#include "builtins.h"

#define MAX_INPUT 1024

void display_prompt() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("myshell:%s$ ", cwd);
    } else {
        perror("getcwd");
        printf("myshell$ ");
    }
}

int main() {
    char input[MAX_INPUT];
    char** args;

    while (1) {
        display_prompt();

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break; // End of input (Ctrl+D)
        }

        // Check for empty input
        if (input[0] == '\n') {
            continue;
        }

        input[strcspn(input, "\n")] = 0; // Remove trailing newline

        args = parse_input(input);

        // Check for built-in commands first
        if (!handle_builtin_command(args)) {
            // If not a built-in, execute as external command
            execute_command(args);
        }

        free(args);
    }

    printf("Exiting shell.\n");
    return 0;
}
