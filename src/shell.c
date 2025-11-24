#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT 1024

void display_prompt() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("myshell:%s$ ", cwd);
    } else {
        perror("getcwd");
        printf("myshell$ ");
    }
}

int main() {
    char input[MAX_INPUT];
    char *args[2]; 
    pid_t pid;

    while (1) {
        display_prompt();

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break; // End of input
        }

        // Remove trailing newline
        input[strcspn(input, "\n")] = 0;

        // Check for exit command
        if (strcmp(input, "exit") == 0) {
            break;
        }

        // Simple parsing for a single command without arguments
        args[0] = input;
        args[1] = NULL;

        pid = fork();

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

    printf("Exiting shell.\n");
    return 0;
}
