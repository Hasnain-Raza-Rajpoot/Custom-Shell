# My Custom Shell

This project is a custom Unix-like shell implemented in C. It provides a feature-rich command-line interface, including complex command execution, job control, piping, I/O redirection, and interactive history.

## How to Compile and Run

### Prerequisites
- `gcc` compiler
- `make` build tool
- `readline` library (`sudo apt-get install libreadline-dev` on Debian/Ubuntu)

### Compilation
To build the shell, run the `make` command from the project's root directory:
```bash
make
```
This will compile all source files and place the executable `myshell` in the `bin/` directory.

### Running
To start the shell, execute the compiled binary:
```bash
./bin/myshell
```

To execute a script file:
```bash
./bin/myshell your_script.sh
```

## High-Level Architecture

The shell operates on a classic **Read-Eval-Print Loop (REPL)**. The core logic is orchestrated by `shell.c`, but the architecture is highly modular, with specific responsibilities delegated to different source files.

The lifecycle of a command is as follows:

1.  **Read:** The shell uses the `readline` library to display a prompt and read a line of input. This provides interactive history (up/down arrows) and tab completion.
2.  **Pre-Processing:** The input line is checked for history expansion (`!!`, `!n`) and alias expansion. If an expansion occurs, the original line is replaced.
3.  **Parsing:** The final command line is parsed. The shell first checks for complex structures like pipes (`|`). If none are found, the line is tokenized into a command and its arguments. Environment variables (`$VAR`) and wildcards (`*`) are expanded at this stage.
4.  **Evaluation (Eval):** The shell determines the command type:
    *   **Built-in Command:** If the command is a built-in (e.g., `cd`, `jobs`, `exit`), the corresponding function is executed directly within the shell's process.
    *   **External Command:** If it's not a built-in, the shell forks a child process to execute the command.
5.  **Execution:**
    *   The child process sets up I/O redirection (`<`, `>`), puts itself in a unique process group for job control, and then uses `execvp` to replace its own image with the new program.
    *   The parent shell either waits for the child to complete (for foreground jobs) or immediately returns to the prompt (for background jobs `&`).
6.  **Print & Loop:** The output of the command is printed to the terminal. The shell then cleans up any completed background jobs and displays the prompt for the next command.

---

## Deep-Level Architecture (File Breakdown)

The project is organized into several modules, each with a header (`.h`) in `include/` and an implementation (`.c`) in `src/`.

### `shell.c`
- **Responsibility:** The main orchestrator and entry point of the shell.
- **Key Logic:**
    - Contains the `main()` function.
    - Implements the main `while(1)` REPL loop.
    - Initializes all subsystems (job control, history, completion).
    - Uses `readline` to get user input.
    - Handles history and alias expansion.
    - Detects whether a command is a simple command or a pipe and calls the appropriate handler (`handle_pipe` or `execute_command`).
    - Detects background commands (`&`).

### `parser.c` & `parser.h`
- **Responsibility:** Tokenizing a simple command line string.
- **Key Logic:**
    - The `parse_input()` function takes a string and splits it into an array of arguments (`char**`) based on whitespace.
    - It allocates memory correctly so that the resulting argument array can be safely managed and freed by other parts of the shell.

### `executor.c` & `executor.h`
- **Responsibility:** Executing a single, non-piped external command.
- **Key Logic:**
    - `execute_command()` is the core function. It takes an argument array and a background flag.
    - It uses `fork()` to create a child process.
    - **Parent Process:** Adds the new process to the job list and either waits for it (`put_job_in_foreground`) or continues (`is_background` is true).
    - **Child Process:**
        - Calls `setpgid()` to create a new process group for robust job control.
        - Calls `handle_redirection()` to set up any I/O redirection.
        - Uses `execvp()` to run the command.
        - Includes a fallback to execute scripts that lack a shebang (`#!/bin/...`).

### `builtins.c` & `builtins.h`
- **Responsibility:** Implementing all internal shell commands.
- **Key Logic:**
    - Implements functions for each built-in: `cd`, `pwd`, `help`, `exit`, `jobs`, `fg`, `bg`, `history`, `alias`, `unalias`.
    - `handle_builtin_command()` acts as a dispatcher, checking if a given command matches a built-in and executing it if so. Built-ins run directly in the shell process, which is essential for commands like `cd` and `exit`.

### `pipe.c` & `pipe.h`
- **Responsibility:** Handling single and multi-level pipelines.
- **Key Logic:**
    - `handle_pipe()` is the main function. It splits the input string by the `|` delimiter.
    - It creates a loop that forks a child process for each command in the pipeline.
    - It uses the `pipe()` system call to create a pipe between each child process.
    - It uses `dup2()` to redirect the `stdout` of one command to the `stdin` of the next.
    - The parent process waits for all children in the pipeline to complete.

### `redirect.c` & `redirect.h`
- **Responsibility:** Managing I/O redirection.
- **Key Logic:**
    - `handle_redirection()` scans the argument list for `<`, `>`, and `>>`.
    - It uses `open()` to get file descriptors for the target files and `dup2()` to redirect `STDIN_FILENO` or `STDOUT_FILENO`.
    - It's designed to handle multiple redirections in a single command (e.g., `cmd < in.txt > out.txt`).

### `jobs.c` & `jobs.h`
- **Responsibility:** The core of the job control system.
- **Key Logic:**
    - Defines the `Job` struct and maintains a global `jobs` array.
    - `init_job_control()`: Sets up the shell to take control of the terminal (`tcsetpgrp`).
    - `add_job()` / `remove_job()`: Manages the job list.
    - `put_job_in_foreground()` / `put_job_in_background()`: These functions manage the complex logic of passing terminal control to a child process, waiting for it with `waitpid`, and regaining control.

### `signals.c` & `signals.h`
- **Responsibility:** Handling signals like `Ctrl+C` and `Ctrl+Z`.
- **Key Logic:**
    - `setup_signal_handlers()` sets up custom handlers.
    - `sigint_handler()` (`Ctrl+C`): Sends `SIGINT` to the current foreground job.
    - `sigtstp_handler()` (`Ctrl+Z`): Sends `SIGTSTP` to the current foreground job, stopping it.
    - `sigchld_handler()`: Catches signals from child processes when their state changes (e.g., a background job completes).

### `history.c` & `history.h`
- **Responsibility:** Command history management.
- **Key Logic:**
    - `load_history()` / `save_history()`: Reads from and writes to `~/.myshell_history`.
    - `builtin_history()`: Implements the `history` command.
    - It relies on the `readline` library for the actual storage and retrieval of history entries.

### `expansion.c` & `expansion.h`
- **Responsibility:** Expanding variables and wildcards.
- **Key Logic:**
    - `expand_variables()` uses the powerful POSIX `wordexp()` function. This function takes a command string and automatically handles environment variable expansion (`$VAR`) and wildcard/glob expansion (`*.c`).

### `completion.c` & `completion.h`
- **Responsibility:** Interactive tab completion.
- **Key Logic:**
    - `initialize_completion()` registers custom completion functions with the `readline` library.
    - `build_command_list()`: At startup, this function scans every directory in the `$PATH` to build a comprehensive list of all available executable commands.
    - `command_generator()`: This function is called by `readline` when the user presses `Tab`. It provides matching commands from the pre-built list. If not completing a command, it lets `readline` fall back to its default filename completion.

### `Makefile`
- **Responsibility:** Compiling and linking the entire project.
- **Key Logic:**
    - Defines rules for compiling `.c` files into `.o` object files.
    - Links all object files together into the final `myshell` executable.
    - Includes `-lreadline` to link against the readline library.
    - Provides a `clean` rule to remove build artifacts.
