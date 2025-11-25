#ifndef REDIRECT_H
#define REDIRECT_H

/**
 * Scans for and handles I/O redirection operators ('<', '>', '>>').
 * This function should be called in the child process before execvp.
 * It modifies the args array by setting redirection operators and filenames to NULL,
 * effectively removing them from the command's arguments.
 *
 * @param args The parsed command and arguments.
 */
void handle_redirection(char** args);

#endif //REDIRECT_H
