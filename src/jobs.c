#include "jobs.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <sys/wait.h> // For waitpid, WUNTRACED, WCONTINUED
#include <errno.h>    // For errno, ECHILD

Job jobs[MAX_JOBS];
int next_job_id = 1;
pid_t shell_pgid;
int shell_is_interactive;
struct termios shell_tmodes;
int shell_terminal;
int current_foreground_job = -1; // job_id of the current foreground job

void init_job_control() {
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty(shell_terminal);

    if (shell_is_interactive) {
        // Loop until we are the foreground process group
        while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp())) {
            kill(-shell_pgid, SIGTTIN);
        }

        // Ignore interactive and job-control signals
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGCHLD, SIG_IGN); // We'll handle SIGCHLD explicitly in main loop or separate function

        // Put ourselves in our own process group
        shell_pgid = getpid();
        if (setpgid(shell_pgid, shell_pgid) < 0) {
            perror("Couldn't put the shell in its own process group");
            exit(1);
        }

        // Grab control of the terminal
        tcsetpgrp(shell_terminal, shell_pgid);

        // Save default terminal attributes for shell
        tcgetattr(shell_terminal, &shell_tmodes);
    }
}

void cleanup_jobs() {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid != 0 && (jobs[i].status == COMPLETED || jobs[i].status == TERMINATED)) {
            // Optionally print a message about the job finishing
            // printf("[%d] %s %s\n", jobs[i].job_id, jobs[i].status == COMPLETED ? "Done" : "Terminated", jobs[i].command);
            jobs[i].pid = 0;
            jobs[i].job_id = 0;
        }
    }
}

void add_job(pid_t pid, pid_t pgid, const char* command, enum JobStatus status, int is_background) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid == 0) {
            jobs[i].pid = pid;
            jobs[i].pgid = pgid;
            strncpy(jobs[i].command, command, sizeof(jobs[i].command) - 1);
            jobs[i].command[sizeof(jobs[i].command) - 1] = '\0';
            jobs[i].status = status;
            jobs[i].job_id = next_job_id++;
            jobs[i].is_background = is_background;

            if (is_background) {
                printf("[%d] %d\n", jobs[i].job_id, jobs[i].pid);
            }
            return;
        }
    }
    fprintf(stderr, "Too many jobs.\n");
}

void remove_job(int job_id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id == job_id) {
            jobs[i].pid = 0;
            jobs[i].job_id = 0;
            // Potentially shift other jobs down or just leave gap
            return;
        }
    }
}

Job* get_job_by_pid(pid_t pid) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid == pid) {
            return &jobs[i];
        }
    }
    return NULL;
}

Job* get_job_by_job_id(int job_id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id == job_id) {
            return &jobs[i];
        }
    }
    return NULL;
}

void update_job_status(pid_t pid, enum JobStatus status) {
    Job* job = get_job_by_pid(pid);
    if (job) {
        job->status = status;
    }
}

void print_jobs() {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid != 0) {
            char status_str[20];
            switch (jobs[i].status) {
                case RUNNING:
                case FOREGROUND:
                case BACKGROUND:
                    strcpy(status_str, "Running");
                    break;
                case STOPPED:
                    strcpy(status_str, "Stopped");
                    break;
                case COMPLETED:
                    strcpy(status_str, "Done");
                    break;
                case TERMINATED:
                    strcpy(status_str, "Terminated");
                    break;
            }
            printf("[%d] %s %s\n", jobs[i].job_id, status_str, jobs[i].command);
        }
    }
}

void put_job_in_foreground(Job* job, int cont) {
    if (!shell_is_interactive || !job) return;

    current_foreground_job = job->job_id;

    // Send the job to the foreground
    tcsetpgrp(shell_terminal, job->pgid);

    // Send SIGCONT to a stopped job
    if (cont && job->status == STOPPED) {
        if (kill(-job->pgid, SIGCONT) < 0) {
            perror("kill (SIGCONT)");
        }
    }

    job->status = FOREGROUND;

    // Wait for the job to complete or stop
    int status;
    pid_t wpid;
    do {
        wpid = waitpid(job->pid, &status, WUNTRACED | WCONTINUED);
        if (wpid == -1 && errno != ECHILD) {
            perror("waitpid");
        }

        if (WIFEXITED(status)) {
            update_job_status(job->pid, COMPLETED);
            printf("[%d] Done %s\n", job->job_id, job->command);
            remove_job(job->job_id);
        } else if (WIFSIGNALED(status)) {
            update_job_status(job->pid, TERMINATED);
            printf("[%d] Terminated %s\n", job->job_id, job->command);
            remove_job(job->job_id);
        } else if (WIFSTOPPED(status)) {
            update_job_status(job->pid, STOPPED);
            printf("[%d] Stopped %s\n", job->job_id, job->command);
        } else if (WIFCONTINUED(status)) {
            update_job_status(job->pid, RUNNING); // Or BACKGROUND/FOREGROUND based on context
        }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status) && !WIFSTOPPED(status));

    // Give the terminal back to the shell
    tcsetpgrp(shell_terminal, shell_pgid);
    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);

    current_foreground_job = -1;
}

void put_job_in_background(Job* job, int cont) {
    if (!shell_is_interactive || !job) return;

    // If it was stopped, continue it in the background
    if (cont && job->status == STOPPED) {
        if (kill(-job->pgid, SIGCONT) < 0) {
            perror("kill (SIGCONT)");
        }
    }
    job->status = BACKGROUND;
    job->is_background = 1;
    printf("[%d] %s %s\n", job->job_id, (cont && job->status == STOPPED) ? "Continuing" : "Running", job->command);
}

// Built-in functions
void builtin_jobs(char** args) {
    print_jobs();
}

void builtin_fg(char** args) {
    if (args[1] == NULL) {
        fprintf(stderr, "fg: usage: fg <job_id>\n");
        return;
    }

    int job_id = atoi(args[1]);
    Job* job = get_job_by_job_id(job_id);

    if (job == NULL) {
        fprintf(stderr, "fg: no such job: %d\n", job_id);
        return;
    }
    put_job_in_foreground(job, 1);
}

void builtin_bg(char** args) {
    if (args[1] == NULL) {
        fprintf(stderr, "bg: usage: bg <job_id>\n");
        return;
    }

    int job_id = atoi(args[1]);
    Job* job = get_job_by_job_id(job_id);

    if (job == NULL) {
        fprintf(stderr, "bg: no such job: %d\n", job_id);
        return;
    }
    put_job_in_background(job, 1);
}
