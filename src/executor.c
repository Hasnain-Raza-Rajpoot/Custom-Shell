#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void execute_command(char** args) {
    // Handle empty command
    if (args[0] == NULL) {
        return;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror(args[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        wait(NULL);
    }
}
