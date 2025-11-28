#ifndef ALIAS_H
#define ALIAS_H

// Built-in commands for managing aliases
void builtin_alias(char** args);
void builtin_unalias(char** args);

// Function to expand an alias if it exists
char* expand_alias(const char* command_name);

#endif //ALIAS_H
