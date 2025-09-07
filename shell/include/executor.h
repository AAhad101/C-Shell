#ifndef EXECUTOR_H
#define EXECUTOR_H

int execute_shell_cmd(ShellCmdNode *shell_cmd, char **pwd, char *shell_dir, char *command, int *job_number);

int execute_cmd_group(CmdGroupNode *cmd_group, char **pwd, char *shell_dir, int *job_number);

int execute_atomic(AtomicNode *atomic, char **pwd, char *shell_dir, int *job_number);

char *int_to_str(int number);

void print_completed_bg(char *shell_dir);

int num_len(int num);

#endif