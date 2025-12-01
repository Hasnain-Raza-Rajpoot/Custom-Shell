#include "completion.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include "builtins.h"

static char** shell_completion(const char* text, int start, int end);
static char* command_generator(const char* text, int state);

// A list to hold all possible commands (built-ins + executables from PATH)
static char** command_list = NULL;
static int command_count = 0;

void build_command_list();
void free_command_list();

void initialize_completion() {
    rl_attempted_completion_function = shell_completion;
    // Build the command list once at the start
    build_command_list();
    // Register a function to free the list on exit
    atexit(free_command_list);
}

static char** shell_completion(const char* text, int start, int end) {
    // If we are at the beginning of a command, use our command generator
    if (start == 0) {
        return rl_completion_matches(text, command_generator);
    }
    // Otherwise, let readline do its default filename completion
    return NULL;
}

static char* command_generator(const char* text, int state) {
    static int list_index, len;

    if (!state) { // First call for this completion
        list_index = 0;
        len = strlen(text);
    }

    // Return the next command from our list that matches the text
    while (list_index < command_count) {
        const char* name = command_list[list_index];
        list_index++;
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    return NULL; // No more matches
}

// Comparison function for qsort
int compare_strings(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

void build_command_list() {
    // Add built-in commands
    extern const char* builtin_names[];
    extern int num_builtins();
    command_count = num_builtins();

    // Start with a reasonable allocation size
    int capacity = command_count + 256;
    command_list = malloc(capacity * sizeof(char*));

    for (int i = 0; i < command_count; i++) {
        command_list[i] = strdup(builtin_names[i]);
    }

    // Add executables from PATH
    char* path_env = getenv("PATH");
    if (path_env == NULL) return;

    char* path = strdup(path_env); // Make a copy for strtok
    char* token = strtok(path, ":");

    while (token != NULL) {
        DIR* dir = opendir(token);
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != NULL) {
                // Ignore '.' and '..'
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }
                
                // Construct full path to check if it's executable
                char full_path[1024];
                snprintf(full_path, sizeof(full_path), "%s/%s", token, entry->d_name);
                
                struct stat st;
                if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR)) {
                    if (command_count >= capacity) {
                        capacity *= 2;
                        command_list = realloc(command_list, capacity * sizeof(char*));
                    }
                    command_list[command_count++] = strdup(entry->d_name);
                }
            }
            closedir(dir);
        }
        token = strtok(NULL, ":");
    }
    free(path);

    // Sort the list for nice, alphabetical completion
    qsort(command_list, command_count, sizeof(char*), compare_strings);
}

void free_command_list() {
    if (command_list) {
        for (int i = 0; i < command_count; i++) {
            free(command_list[i]);
        }
        free(command_list);
    }
}