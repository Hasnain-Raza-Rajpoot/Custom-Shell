#include "pipe.h"
#include "parser.h"
#include "builtins.h"
#include "redirect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_COMMANDS 16 // Maximum number of piped commands

void handle_pipe(char* input) {
    char* commands[MAX_COMMANDS];
    int num_commands = 0;

    // Split the input string into commands based on the pipe character
    char* token = strtok(input, "|");
    while (token != NULL && num_commands < MAX_COMMANDS) {
        commands[num_commands++] = token;
        token = strtok(NULL, "|");
    }
    
    if (num_commands == 0) {
        return;
    }

    int prev_pipe_read_end = -1;
    pid_t pids[MAX_COMMANDS];

    for (int i = 0; i < num_commands; i++) {
        int pipefd[2];

        // Create a pipe for all but the last command
        if (i < num_commands - 1) {
            if (pipe(pipefd) < 0) {
                perror("pipe");
                return;
            }
        }

        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            return;
        }

        if (pids[i] == 0) {
            // --- Child Process ---

            // If not the first command, redirect stdin from the previous pipe
            if (prev_pipe_read_end != -1) {
                dup2(prev_pipe_read_end, STDIN_FILENO);
                close(prev_pipe_read_end);
            }

            // If not the last command, redirect stdout to the new pipe
            if (i < num_commands - 1) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]); // Close the write end in the child
                close(pipefd[0]); // Close the read end in the child (it's for the next command)
            }
            
            char** args = parse_input(commands[i]);
            
            handle_redirection(args); // Handle redirection for this part of the pipe

            // Note: Built-ins in a pipe won't work with this structure
            // because they don't use execvp. This is an advanced feature.
            if (args[0] != NULL) {
                 if (execvp(args[0], args) == -1) {
                    perror(args[0]);
                    exit(EXIT_FAILURE);
                }
            }
            exit(EXIT_SUCCESS); // Exit if command was empty
        }

        // --- Parent Process ---

        // Close the previous pipe's read end, it's been passed on
        if (prev_pipe_read_end != -1) {
            close(prev_pipe_read_end);
        }

        // If not the last command, save the read end for the next child
        if (i < num_commands - 1) {
            close(pipefd[1]); // Close the write end in the parent
            prev_pipe_read_end = pipefd[0];
        }
    }

    // Wait for all child processes to complete
    for (int i = 0; i < num_commands; i++) {
        waitpid(pids[i], NULL, 0);
    }
}