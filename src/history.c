#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

// Built-in history command
void builtin_history(char** args) {
    HIST_ENTRY** hist_list = history_list();
    if (hist_list) {
        for (int i = 0; hist_list[i]; i++) {
            printf("%d  %s\n", i+history_base, hist_list[i]->line);
        }
    }
}

// Load history from file
void load_history() {
    char* home_dir = getenv("HOME");
    if (home_dir == NULL) {
        fprintf(stderr, "HOME environment variable not set, cannot load history.\n");
        return;
    }

    char history_path[1024];
    snprintf(history_path, sizeof(history_path), "%s/%s", home_dir, HISTORY_FILE);

    read_history(history_path);
}

// Save history to file
void save_history() {
    char* home_dir = getenv("HOME");
    if (home_dir == NULL) {
        fprintf(stderr, "HOME environment variable not set, cannot save history.\n");
        return;
    }

    char history_path[1024];
    snprintf(history_path, sizeof(history_path), "%s/%s", home_dir, HISTORY_FILE);
    
    write_history(history_path);
}
