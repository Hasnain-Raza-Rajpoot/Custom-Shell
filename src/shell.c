#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "parser.h"
#include "executor.h"
#include "builtins.h"
#include "pipe.h"
#include "jobs.h"
#include "signals.h"

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
    int is_background = 0;

    init_job_control();
    setup_signal_handlers();

    while (1) {
        cleanup_jobs(); // Clean up completed background jobs

        display_prompt();

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break; // End of input (Ctrl+D)
        }

        if (input[0] == '\n') {
            continue;
        }

        input[strcspn(input, "\n")] = 0; // Remove trailing newline

        // Check for background command
        if (strlen(input) > 0 && input[strlen(input) - 1] == '&') {
            is_background = 1;
            input[strlen(input) - 1] = '\0'; // Remove the &
        } else {
            is_background = 0;
        }

        // Check for pipe character
        if (strchr(input, '|')) {
            // Backgrounding piped commands is complex and will be considered an advanced feature.
            // For now, piped commands always run in foreground.
            handle_pipe(input); 
        } else {
            args = parse_input(input);

            // Handle empty command after & removal
            if (args[0] == NULL) {
                free(args);
                continue;
            }

            // Check for built-in commands first
            // Built-ins cannot be run in background with this simple scheme
            if (!handle_builtin_command(args)) {
                // If not a built-in, execute as external command
                execute_command(args, is_background);
            }
            free(args);
        }
    }

    printf("Exiting shell.\n");
    return 0;
}
