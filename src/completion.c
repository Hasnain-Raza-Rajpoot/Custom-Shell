#include "completion.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "builtins.h"

static char** command_completion(const char* text, int start, int end);
static char* command_generator(const char* text, int state);

void initialize_completion() {
    // Tell readline that we are handling completion
    rl_attempted_completion_function = command_completion;
}

// This is the main completion function called by readline
static char** command_completion(const char* text, int start, int end) {
    // If the cursor is at the beginning of the line, we do command completion
    if (start == 0) {
        return rl_completion_matches(text, command_generator);
    }

    // Otherwise, do default filename completion
    // Returning NULL tells readline to use its default behavior (filename completion)
    return NULL;
}

// This function generates potential command matches
static char* command_generator(const char* text, int state) {
    static int list_index, len;
    static const char** command_list = NULL;

    // If this is a new word to complete, build the list of possible commands
    if (!state) {
        list_index = 0;
        len = strlen(text);

        // For simplicity, we'll just use our built-ins.
        // A more advanced version would scan the entire $PATH.
        // For now, this demonstrates the mechanism.
        
        // This is a simple example. A real implementation would be more dynamic.
        // Let's use the built-ins defined in builtins.c
        extern const char* builtin_names[];
        extern int num_builtins();
        command_list = builtin_names; 
    }

    // Return the next name that matches
    const char* name;
    while ((name = command_list[list_index++])) {
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    return NULL; // No more matches
}
