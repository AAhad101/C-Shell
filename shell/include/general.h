#ifndef GENERAL_H
#define GENERAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <linux/limits.h>
#include <dirent.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include "parser.h"

#define CMD_MAX 4096

typedef enum Status{
    RUNNING,
    STOPPED,
    TERMINATED
} Status;

typedef struct BG_process{
    pid_t pid;
    int job_no;
    char *command;
    Status status;
} BG_process;

char *int_to_str(int number);

int num_len(int num);

int pow_ten(int pow);

int str_to_int(char *str);

char *stringify_atomic(AtomicNode *atomic_cmd);

char *stringify_cmd_group(CmdGroupNode *cmd_group);

#endif


