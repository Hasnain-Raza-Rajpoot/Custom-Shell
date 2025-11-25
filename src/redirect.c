#include "redirect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

void handle_redirection(char** args) {
    // --- Pass 1: Apply all redirections ---
    for (int i = 0; args[i] != NULL; i++) {
        // Handle output redirection (truncate)
        if (strcmp(args[i], ">") == 0) {
            if (args[i+1] == NULL) {
                fprintf(stderr, "syntax error near unexpected token `newline'\n");
                exit(EXIT_FAILURE);
            }
            int fd_out = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out < 0) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
        // Handle output redirection (append)
        else if (strcmp(args[i], ">>") == 0) {
            if (args[i+1] == NULL) {
                fprintf(stderr, "syntax error near unexpected token `newline'\n");
                exit(EXIT_FAILURE);
            }
            int fd_out = open(args[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd_out < 0) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
        // Handle input redirection
        else if (strcmp(args[i], "<") == 0) {
             if (args[i+1] == NULL) {
                fprintf(stderr, "syntax error near unexpected token `newline'\n");
                exit(EXIT_FAILURE);
            }
            int fd_in = open(args[i+1], O_RDONLY);
            if (fd_in < 0) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
    }

    // --- Pass 2: Clean up args for execvp ---
    int j = 0; // write index
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0) {
            i++; // Skip the operator and the filename that follows
        } else {
            args[j] = args[i];
            j++;
        }
    }
    args[j] = NULL; // Null-terminate the new, cleaned-up argument list
}