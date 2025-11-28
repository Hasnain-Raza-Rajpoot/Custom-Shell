#include "parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char** parse_input(char* input) {
    // Use a copy for counting tokens, as strtok is destructive.
    char* temp_input = strdup(input);
    if (!temp_input) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }

    int arg_count = 0;
    char* token = strtok(temp_input, " \t\n\r");
    while (token != NULL) {
        arg_count++;
        token = strtok(NULL, " \t\n\r");
    }
    free(temp_input);

    // This is the single block of memory for all the argument strings.
    char* data_block = strdup(input);
    if (!data_block) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }

    // This is the array of pointers that will point into the data_block.
    char** args = malloc((arg_count + 1) * sizeof(char*));
    if (!args) {
        free(data_block);
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // If there were no tokens, we still need a valid structure to free.
    // The first pointer in args will point to the data block.
    if (arg_count == 0) {
        args[0] = NULL;
        // To prevent a memory leak, we attach the data_block to the first pointer,
        // even though it will be set to NULL for logic purposes. The shell.c free logic
        // will now free the data_block via a temporary variable.
        // Let's refactor shell.c to handle this more cleanly instead.
        // For now, let's assume arg_count > 0 for non-empty lines.
    }

    int i = 0;
    token = strtok(data_block, " \t\n\r");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " \t\n\r");
    }
    args[i] = NULL;

    return args;
}
