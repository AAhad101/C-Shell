#ifndef HOP_H
#define HOP_H

#include "parser.h"

int hop_verify(AtomicNode *atomic_cmd);

void execute_hop(AtomicNode *atomic_cmd, char **pwd, char *shell_dir);

#endif