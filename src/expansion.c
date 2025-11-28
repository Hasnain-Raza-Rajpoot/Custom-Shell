#include "expansion.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>

// This function now returns a new, correctly allocated argument array.
// The caller is responsible for freeing the returned array.
char** expand_variables(char** args) {
    if (args == NULL || args[0] == NULL) {
        return args;
    }

    // Reconstruct the full command line from the arguments.
    // This is necessary because wordexp works on a single string.
    size_t len = 0;
    for (int i = 0; args[i] != NULL; i++) {
        len += strlen(args[i]) + 1; // +1 for space or null terminator
    }

    char* cmd_line = malloc(len);
    if (!cmd_line) {
        perror("malloc");
        return args;
    }
    cmd_line[0] = '\0';

    for (int i = 0; args[i] != NULL; i++) {
        strcat(cmd_line, args[i]);
        if (args[i+1] != NULL) {
            strcat(cmd_line, " ");
        }
    }

    wordexp_t p;
    // WRDE_NOCMD prevents command substitution for security.
    // WRDE_REUSE can be used to reuse the wordexp_t struct.
    if (wordexp(cmd_line, &p, 0) != 0) {
        // Expansion failed, return original args
        free(cmd_line);
        return args;
    }
    
    // wordexp was successful. We need to create a new args array
    // that the rest of our shell can use and free normally.

    // Free the original, unexpanded args from parse_input
    free(args[0]); // The data block
    free(args);    // The pointer array

    // Allocate a new array of pointers
    char** new_args = malloc((p.we_wordc + 1) * sizeof(char*));
    if (!new_args) {
        perror("malloc");
        wordfree(&p);
        free(cmd_line);
        return NULL;
    }
    
    // Calculate total size for the new argument strings
    size_t total_arg_len = 0;
    for (size_t i = 0; i < p.we_wordc; i++) {
        total_arg_len += strlen(p.we_wordv[i]) + 1;
    }

    // Allocate a single block for all argument strings
    char* new_args_data = malloc(total_arg_len);
     if (!new_args_data) {
        perror("malloc");
        free(new_args);
        wordfree(&p);
        free(cmd_line);
        return NULL;
    }

    // Copy the strings from wordexp results into our new block
    char* current_pos = new_args_data;
    for (size_t i = 0; i < p.we_wordc; i++) {
        strcpy(current_pos, p.we_wordv[i]);
        new_args[i] = current_pos;
        current_pos += strlen(p.we_wordv[i]) + 1;
    }
    new_args[p.we_wordc] = NULL;

    wordfree(&p);
    free(cmd_line);

    return new_args;
}