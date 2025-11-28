#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "jobs.h"     // For job control built-ins
#include "history.h"  // For history built-in
#include "alias.h"    // For alias built-ins

// Forward declarations for built-in functions
void builtin_cd(char** args);
void builtin_pwd(char** args);
void builtin_help(char** args);
void builtin_exit(char** args);
void builtin_jobs(char** args);
void builtin_fg(char** args);
void builtin_bg(char** args);
void builtin_history(char** args);
// New built-in declarations
void builtin_alias(char** args);
void builtin_unalias(char** args);

// Array of built-in command names
const char* builtin_names[] = {
    "cd",
    "pwd",
    "help",
    "exit",
    "jobs",
    "fg",
    "bg",
    "history", // New built-in
    "alias",   // New built-in
    "unalias"  // New built-in
};

// Array of corresponding built-in functions
void (*builtin_funcs[]) (char**) = {
    &builtin_cd,
    &builtin_pwd,
    &builtin_help,
    &builtin_exit,
    &builtin_jobs,
    &builtin_fg,
    &builtin_bg,
    &builtin_history, // New built-in
    &builtin_alias,
    &builtin_unalias
};

int num_builtins() {
    return sizeof(builtin_names) / sizeof(char*);
}

void builtin_cd(char** args) {
    if (args[1] == NULL) {
        // No argument, change to HOME directory
        char* home = getenv("HOME");
        if (home == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return;
        }
        if (chdir(home) != 0) {
            perror("cd");
        }
    } else {
        if (chdir(args[1]) != 0) {
            perror("cd");
        }
    }
}

void builtin_pwd(char** args) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd");
    }
}

void builtin_help(char** args) {
    printf("My Custom Shell\n");
    printf("The following built-in commands are available:\n");
    for (int i = 0; i < num_builtins(); i++) {
        printf("  %s\n", builtin_names[i]);
    }
}

void builtin_exit(char** args) {
    exit(0);
}

int handle_builtin_command(char** args) {
    if (args[0] == NULL) {
        // An empty command is not a built-in
        return 0;
    }

    for (int i = 0; i < num_builtins(); i++) {
        if (strcmp(args[0], builtin_names[i]) == 0) {
            (*builtin_funcs[i])(args);
            return 1; // It was a built-in, and we handled it
        }
    }

    return 0; // Not a built-in command
}
