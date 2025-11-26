#ifndef HISTORY_H
#define HISTORY_H

#define HISTORY_FILE ".myshell_history"

void builtin_history(char** args);
void load_history();
void save_history();

#endif //HISTORY_H
