#include "signals.h"
#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

// Handler for SIGINT (Ctrl+C)
void sigint_handler(int signo) {
    if (current_foreground_job != -1) {
        Job* job = get_job_by_job_id(current_foreground_job);
        if (job) {
            fprintf(stderr, "\nSending SIGINT to foreground job [%d] %s\n", job->job_id, job->command);
            if (kill(-job->pgid, SIGINT) < 0) {
                perror("kill (SIGINT)");
            }
        }
    } else {
        // If no foreground job, let the default behavior (terminate shell) happen
        // or you can choose to ignore, but usually Ctrl+C should terminate shell if no foreground job.
        // For now, let's re-raise it for the shell to handle.
        // This part might need refinement based on desired shell behavior.
        signal(SIGINT, SIG_DFL);
        kill(getpid(), SIGINT);
    }
}

// Handler for SIGTSTP (Ctrl+Z)
void sigtstp_handler(int signo) {
    if (current_foreground_job != -1) {
        Job* job = get_job_by_job_id(current_foreground_job);
        if (job) {
            fprintf(stderr, "\nSending SIGTSTP to foreground job [%d] %s\n", job->job_id, job->command);
            if (kill(-job->pgid, SIGTSTP) < 0) {
                perror("kill (SIGTSTP)");
            }
        }
    } else {
        // If no foreground job, ignore or re-raise for shell
        // For now, let's just ignore it for the shell itself when no job is in foreground.
    }
}

// Handler for SIGCHLD (Child process status changed)
void sigchld_handler(int signo) {
    pid_t pid;
    int status;

    // Use WNOHANG to prevent blocking, as this is a signal handler
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        Job* job = get_job_by_pid(pid);
        if (!job) {
            // Might be a process not managed by our job control (e.g. from a script or another shell)
            continue;
        }

        if (WIFEXITED(status)) {
            update_job_status(pid, COMPLETED);
            // printf("[%d] Done %s\n", job->job_id, job->command); // Output might interfere with current process
        } else if (WIFSIGNALED(status)) {
            update_job_status(pid, TERMINATED);
            // printf("[%d] Terminated %s\n", job->job_id, job->command);
        } else if (WIFSTOPPED(status)) {
            update_job_status(pid, STOPPED);
            // printf("[%d] Stopped %s\n", job->job_id, job->command);
        } else if (WIFCONTINUED(status)) {
            update_job_status(pid, RUNNING);
        }
    }
}

void setup_signal_handlers() {
    // Setup SIGCHLD handler for cleaning up background processes
    struct sigaction sa_chld;
    memset(&sa_chld, 0, sizeof(sa_chld));
    sa_chld.sa_handler = sigchld_handler;
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP; // SA_NOCLDSTOP prevents SIGCHLD for stopped children, we need it
    if (sigaction(SIGCHLD, &sa_chld, 0) == -1) {
        perror("sigaction SIGCHLD");
    }

    // These are for the shell to pass to foreground jobs
    // For the shell itself, they are ignored in init_job_control
    struct sigaction sa_int;
    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
    }

    struct sigaction sa_tstp;
    memset(&sa_tstp, 0, sizeof(sa_tstp));
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    if (sigaction(SIGTSTP, &sa_tstp, NULL) == -1) {
        perror("sigaction SIGTSTP");
    }
}
