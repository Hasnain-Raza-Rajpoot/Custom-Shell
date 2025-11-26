#include "parser.h"
#include <string.h>
#include <stdlib.h>
#include<stdio.h>

char** parse_input(char* input) {
    char** args = malloc(MAX_ARGS * sizeof(char*));
    if (args == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    
    char* token;
    int i = 0;
    
    token = strtok(input, " \t\n\r");
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i] = token;
        i++;
        token = strtok(NULL, " \t\n\r");
    }
    args[i] = NULL; // Null-terminate the array
    
    return args;
}