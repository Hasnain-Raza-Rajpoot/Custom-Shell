#include "pipe.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void handle_pipe(char* input) {
    char* command1;
    char* command2;
    char** args1;
    char** args2;

    // Find the pipe character and split the input string
    command2 = strchr(input, '|');
    if (command2 == NULL) {
        fprintf(stderr, "Syntax error: expected '|'\n");
        return;
    }
    
    // Replace '|' with a null terminator to separate the commands
    *command2 = '\0';
    command1 = input;
    command2++; // Move to the start of the second command

    // Parse the two commands
    args1 = parse_input(command1);
    args2 = parse_input(command2);

    // Check for empty commands
    if (args1[0] == NULL || args2[0] == NULL) {
        fprintf(stderr, "Invalid null command.\n");
        free(args1);
        free(args2);
        return;
    }

    int pipefd[2];
    pid_t pid1, pid2;

    if (pipe(pipefd) < 0) {
        perror("pipe");
        free(args1);
        free(args2);
        return;
    }

    pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        free(args1);
        free(args2);
        return;
    }

    if (pid1 == 0) {
        // Child 1 (executes command1)
        close(pipefd[0]); // Close read end
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to pipe
        close(pipefd[1]); // Close write end

        execvp(args1[0], args1);
        perror(args1[0]); // execvp only returns on error
        exit(EXIT_FAILURE);
    }

    pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        free(args1);
        free(args2);
        return;
    }

    if (pid2 == 0) {
        // Child 2 (executes command2)
        close(pipefd[1]); // Close write end
        dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to pipe
        close(pipefd[0]); // Close read end

        execvp(args2[0], args2);
        perror(args2[0]); // execvp only returns on error
        exit(EXIT_FAILURE);
    }

    // Parent process
    close(pipefd[0]);
    close(pipefd[1]);

    // Wait for both children to finish
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    free(args1);
    free(args2);
}
