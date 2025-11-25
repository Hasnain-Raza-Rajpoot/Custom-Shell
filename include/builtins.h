#ifndef BUILTINS_H
#define BUILTINS_H

/**
 * Attempts to execute a built-in command.
 * @param args Parsed command and arguments.
 * @return 1 if the command was a built-in and was handled, 0 otherwise.
 */
int handle_builtin_command(char** args);

#endif //BUILTINS_H
