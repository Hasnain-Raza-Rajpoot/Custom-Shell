#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>
#include <termios.h> // For struct termios

#define MAX_JOBS 20

enum JobStatus {
    RUNNING,
    STOPPED,
    BACKGROUND,
    FOREGROUND,
    COMPLETED,
    TERMINATED
};

typedef struct {
    pid_t pid;          // Process ID of the job's process group leader
    pid_t pgid;         // Process group ID
    char command[1024]; // Command string
    enum JobStatus status;
    int job_id;         // Sequential job number
    int is_background;  // 1 if background, 0 if foreground
} Job;

extern Job jobs[MAX_JOBS];
extern int next_job_id;
extern pid_t shell_pgid;
extern int shell_is_interactive;
extern int shell_terminal; // The file descriptor for the shell's terminal
extern struct termios shell_tmodes; // Terminal modes for the shell
extern int current_foreground_job;

void init_job_control();
void cleanup_jobs();
void add_job(pid_t pid, pid_t pgid, const char* command, enum JobStatus status, int is_background);
void remove_job(int job_id);
Job* get_job_by_pid(pid_t pid);
Job* get_job_by_job_id(int job_id);
void update_job_status(pid_t pid, enum JobStatus status);
void print_jobs();
void put_job_in_foreground(Job* job, int cont);
void put_job_in_background(Job* job, int cont);

// Built-in job commands
void builtin_jobs(char** args);
void builtin_fg(char** args);
void builtin_bg(char** args);

#endif //JOBS_H
