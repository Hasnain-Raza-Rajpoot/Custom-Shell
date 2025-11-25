#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "parser.h"
#include "executor.h"

#define MAX_INPUT 1024

void display_prompt() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("\nmyshell:%s$ ", cwd);
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

        // Check for exit command
        if (strcmp(input, "exit") == 0) {
            break;
        }

        args = parse_input(input);
        execute_command(args);

        free(args);
    }

    printf("Exiting shell.\n");
    return 0;
}
