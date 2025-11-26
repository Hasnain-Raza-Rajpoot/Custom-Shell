#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "redirect.h"
#include "jobs.h"
#include <signal.h>

void execute_command(char** args, int is_background) {
    if (args[0] == NULL) {
        return;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        // Child process
        pid_t pgid = getpid(); // Each new process becomes a process group leader
        if (setpgid(pgid, pgid) < 0) {
            perror("setpgid");
            exit(EXIT_FAILURE);
        }

        if (!is_background) {
            // Give the child control of the terminal if it's a foreground job
            if (shell_is_interactive) {
                tcsetpgrp(shell_terminal, pgid);
            }
        }

        // Restore default signal handlers for child processes
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_IGN); // Ignored by default in bash, or SIG_DFL if you want core dump
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        handle_redirection(args); // Check for and apply I/O redirection
        if (execvp(args[0], args) == -1) {
            perror(args[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        pid_t pgid = pid; // For a simple command, the child's PID is its PGID

        // Add job to job list
        add_job(pid, pgid, args[0], is_background ? BACKGROUND : FOREGROUND, is_background);

        if (!is_background) {
            Job* job = get_job_by_pid(pid);
            if (job) {
                put_job_in_foreground(job, 0); // Don't send SIGCONT initially
            }
        }
    }
}
