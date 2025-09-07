#ifndef REVEAL_H
#define REVEAL_H

#include "parser.h"
#define MAX_SIZE 4096

int cmp_strings(const void *name1, const void *name2);

int reveal_verify(AtomicNode *atomic_cmd);

void execute_reveal(AtomicNode *atomic_cmd, char **pwd, char *shell_dir);

#endif