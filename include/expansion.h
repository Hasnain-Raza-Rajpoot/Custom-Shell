#ifndef EXPANSION_H
#define EXPANSION_H

/**
 * Expands environment variables in the argument list.
 * It processes arguments in-place, replacing tokens like $VAR or ${VAR}.
 * @param args The null-terminated array of arguments.
 */
char** expand_variables(char** args);

#endif //EXPANSION_H
