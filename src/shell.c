#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parser.h"
#include "executor.h"
#include "builtins.h"
#include "pipe.h"
#include "jobs.h"
#include "signals.h"
#include "history.h"

#define MAX_INPUT 1024

char* current_prompt_str() {
    static char prompt[1024];
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        snprintf(prompt, sizeof(prompt), "myshell:%s$ ", cwd);
    } else {
        perror("getcwd");
        snprintf(prompt, sizeof(prompt), "myshell$ ");
    }
    return prompt;
}

int main() {
    char* input_line; // The raw line from readline, must be freed
    char** args;
    int is_background = 0;

    init_job_control();
    setup_signal_handlers();
    load_history(); // Load history at startup

    while (1) {
        cleanup_jobs();

        input_line = readline(current_prompt_str());

        if (input_line == NULL) { // Ctrl+D
            printf("\n");
            break;
        }

        if (input_line[0] == '\0') {
            free(input_line);
            continue;
        }

        char* line_to_process = input_line; // Start with the original line

        // --- History Expansion ---
        if (line_to_process[0] == '!') {
            char* expanded_line = NULL;
            HIST_ENTRY* entry = NULL;

            if (line_to_process[1] == '!') { // Handle !!
                if (history_length > 0) entry = history_get(history_length - 1);
            } else { // Handle !n and !-n
                char* endptr;
                long n = strtol(line_to_process + 1, &endptr, 10);
                if (*endptr == '\0') { // Check if it was a valid number
                    if (n > 0) { // !n
                        entry = history_get(n - history_base);
                    } else if (n < 0) { // !-n
                        entry = history_get(history_length + n);
                    }
                }
            }

            if (entry) {
                expanded_line = strdup(entry->line);
                printf("%s\n", expanded_line);
                free(line_to_process); // Free the original "!!", "!n" line
                line_to_process = expanded_line; // Process the expanded line instead
            } else if (line_to_process[1] != '\0') {
                fprintf(stderr, "%s: event not found\n", line_to_process);
                free(line_to_process);
                continue;
            }
        }

        // Add the final, expanded command to history
        if (line_to_process && line_to_process[0] != '\0') {
            add_history(line_to_process);
        }

        // Make a mutable copy for parsing, because strtok is destructive
        char temp_input[MAX_INPUT];
        strncpy(temp_input, line_to_process, sizeof(temp_input) - 1);
        temp_input[sizeof(temp_input) - 1] = '\0';

        // Check for background command
        if (strlen(temp_input) > 0 && temp_input[strlen(temp_input) - 1] == '&') {
            is_background = 1;
            temp_input[strlen(temp_input) - 1] = '\0';
        } else {
            is_background = 0;
        }

        if (strchr(temp_input, '|')) {
            handle_pipe(temp_input);
        } else {
            args = parse_input(temp_input);
            if (args[0] != NULL) {
                if (!handle_builtin_command(args)) {
                    execute_command(args, is_background);
                }
            }
            free(args);
        }

        free(line_to_process); // Free the line from readline() or strdup()
    }

    save_history();
    printf("Exiting shell.\n");
    return 0;
}
