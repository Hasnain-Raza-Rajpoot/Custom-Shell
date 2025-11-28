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
#include "expansion.h"
#include "completion.h"
#include "alias.h"    // New include

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

int main(int argc, char** argv) {
    // --- Script Execution Mode ---
    if (argc > 1) {
        FILE* script_file = fopen(argv[1], "r");
        if (!script_file) {
            perror(argv[1]);
            exit(EXIT_FAILURE);
        }

        char line[MAX_INPUT];
        while (fgets(line, sizeof(line), script_file)) {
            // Basic execution, doesn't handle complex multi-line scripts,
            // backgrounding, or job control in a meaningful way.
            line[strcspn(line, "\n")] = 0; // Remove newline

            char** args = parse_input(line);
            if (args != NULL && args[0] != NULL) {
                args = expand_variables(args);
                if (!handle_builtin_command(args)) {
                    execute_command(args, 0); // Always foreground in scripts
                }
                free(args[0]);
                free(args);
            } else if (args != NULL) {
                free(args);
            }
        }
        fclose(script_file);
        exit(EXIT_SUCCESS);
    }

    // --- Interactive Mode ---
    char* input_line;
    char** args;
    // ... (rest of the interactive main function)

    int is_background = 0;

    init_job_control();
    setup_signal_handlers();
    load_history(); // Load history at startup
    initialize_completion(); // Initialize tab completion

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
                 entry = history_get(history_base + history_length - 1);
            } else { // Handle !n and !-n
                char* endptr;
                long n = strtol(line_to_process + 1, &endptr, 10);
                if (*endptr == '\0') {
                    if (n > 0) { // !n
                        entry = history_get(n);
                    } else if (n < 0) { // !-n
                        entry = history_get(history_base + history_length + n);
                    }
                }
            }

            if (entry) {
                expanded_line = strdup(entry->line);
                printf("%s\n", expanded_line);
                free(line_to_process);
                line_to_process = expanded_line;
            } else if (line_to_process[1] != '\0') {
                fprintf(stderr, "%s: event not found\n", line_to_process);
                free(line_to_process);
                continue;
            }
        }

        // --- Alias Expansion ---
        char* first_word = strndup(line_to_process, strcspn(line_to_process, " \t\n\r"));
        char* expanded_alias = expand_alias(first_word);
        free(first_word);

        if (expanded_alias) {
            char* rest_of_line = strchr(line_to_process, ' ');
            if (rest_of_line == NULL) rest_of_line = "";
            
            char* new_line = malloc(strlen(expanded_alias) + strlen(rest_of_line) + 2);
            sprintf(new_line, "%s %s", expanded_alias, rest_of_line);
            
            free(expanded_alias);
            free(line_to_process);
            line_to_process = new_line;
        }

        // Add the final, expanded command to history
        if (line_to_process && line_to_process[0] != '\0') {
            add_history(line_to_process);
        }


        char temp_input[MAX_INPUT];
        strncpy(temp_input, line_to_process, sizeof(temp_input) - 1);
        temp_input[sizeof(temp_input) - 1] = '\0';

        if (strlen(temp_input) > 0 && temp_input[strlen(temp_input) - 1] == '&') {
            is_background = 1;
            temp_input[strlen(temp_input) - 1] = '\0';
        } else {
            is_background = 0;
        }

        if (strchr(temp_input, '|')) {
            // NOTE: Expansion for pipes would require more complex logic.
            // For now, we skip expansion for pipes.
            handle_pipe(temp_input);
        } else {
            args = parse_input(temp_input);
            if (args != NULL) {
                char* data_block_ptr = (args[0] != NULL) ? args[0] : NULL; // For freeing later

                if (args[0] != NULL) {
                    args = expand_variables(args); // Re-assign args
                    if (!handle_builtin_command(args)) {
                        execute_command(args, is_background);
                    }
                }
                
                // Free the memory allocated by either parse_input or expand_variables
                free(args[0]); // Free the data block
                free(args);    // Free the pointer array
            }
        }

        free(line_to_process);
    }

    save_history();
    printf("Exiting shell.\n");
    return 0;
}
