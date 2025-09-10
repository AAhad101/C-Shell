#ifndef EXECUTOR_H
#define EXECUTOR_H

int execute_shell_cmd(ShellCmdNode *shell_cmd, char **pwd, char *shell_dir, char *command, int *job_number, BG_process **bg_prcs, int *active_bgs);

int execute_cmd_group(CmdGroupNode *cmd_group, char **pwd, char *shell_dir, int *job_number, BG_process **bg_prcs, int *active_bgs, int is_foreground);

int execute_atomic(AtomicNode *atomic, char **pwd, char *shell_dir, int *job_number, BG_process **bg_prcs, int *active_bgs, int is_foreground);

#endif