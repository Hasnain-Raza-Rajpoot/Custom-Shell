#include "alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ALIASES 50

typedef struct {
    char* name;
    char* value;
} Alias;

static Alias alias_list[MAX_ALIASES];
static int alias_count = 0;

void print_aliases() {
    for (int i = 0; i < alias_count; i++) {
        printf("alias %s='%s'\n", alias_list[i].name, alias_list[i].value);
    }
}

void builtin_alias(char** args) {
    if (args[1] == NULL) {
        print_aliases();
        return;
    }

    char* eq_pos = strchr(args[1], '=');
    if (eq_pos == NULL) {
        // Just print the specific alias
        for (int i = 0; i < alias_count; i++) {
            if (strcmp(alias_list[i].name, args[1]) == 0) {
                printf("alias %s='%s'\n", alias_list[i].name, alias_list[i].value);
                return;
            }
        }
        return;
    }
    
    // Define a new alias
    if (alias_count >= MAX_ALIASES) {
        fprintf(stderr, "alias: Too many aliases defined.\n");
        return;
    }

    *eq_pos = '\0'; // Split the string at '='
    char* name = args[1];
    char* value = eq_pos + 1;

    // Remove quotes if present
    if (*value == '\'' || *value == '"') {
        value++;
        char* end_quote = strrchr(value, *value == '\'' ? '\'' : '"');
        if (end_quote) *end_quote = '\0';
    }

    // Check if alias already exists to overwrite it
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(alias_list[i].name, name) == 0) {
            free(alias_list[i].value);
            alias_list[i].value = strdup(value);
            return;
        }
    }

    // Add new alias
    alias_list[alias_count].name = strdup(name);
    alias_list[alias_count].value = strdup(value);
    alias_count++;
}

void builtin_unalias(char** args) {
    if (args[1] == NULL) {
        fprintf(stderr, "unalias: usage: unalias <alias_name>\n");
        return;
    }

    for (int i = 0; i < alias_count; i++) {
        if (strcmp(alias_list[i].name, args[1]) == 0) {
            free(alias_list[i].name);
            free(alias_list[i].value);
            // Shift remaining aliases down
            for (int j = i; j < alias_count - 1; j++) {
                alias_list[j] = alias_list[j+1];
            }
            alias_count--;
            return;
        }
    }
}

char* expand_alias(const char* command_name) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(alias_list[i].name, command_name) == 0) {
            return strdup(alias_list[i].value);
        }
    }
    return NULL;
}
