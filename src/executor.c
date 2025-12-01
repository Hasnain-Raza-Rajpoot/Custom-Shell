#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "redirect.h"
#include "jobs.h"
#include <signal.h>
#include <errno.h> // For errno

void execute_command(char** args, int is_background) {
    if (args[0] == NULL) {
        return;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        // Child process
        pid_t pgid = getpid();
        if (setpgid(pgid, pgid) < 0) {
            perror("setpgid");
            exit(EXIT_FAILURE);
        }

        if (!is_background && shell_is_interactive) {
            tcsetpgrp(shell_terminal, pgid);
        }

        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        handle_redirection(args);
        
        execvp(args[0], args);
        
        // If execvp fails, check if it's an executable script without a shebang
        if (errno == ENOEXEC) {
            // Prepend our shell to the args and re-execute
            // First, count existing args
            int arg_count = 0;
            while(args[arg_count] != NULL) {
                arg_count++;
            }
            // Create new args array
            char** new_args = malloc((arg_count + 2) * sizeof(char*));
            if (!new_args) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            new_args[0] = "./bin/myshell";
            for (int i = 0; i < arg_count; i++) {
                new_args[i+1] = args[i];
            }
            new_args[arg_count + 1] = NULL;
            
            execvp(new_args[0], new_args);
            // If this also fails, print the error for the original command
        }
        
        perror(args[0]);
        exit(EXIT_FAILURE);

    } else {
        // Parent process
        pid_t pgid = pid;
        add_job(pid, pgid, args[0], is_background ? BACKGROUND : FOREGROUND, is_background);

        if (!is_background) {
            Job* job = get_job_by_pid(pid);
            if (job) {
                put_job_in_foreground(job, 0);
            }
        }
    }
}
