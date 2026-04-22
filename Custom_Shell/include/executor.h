#ifndef EXECUTOR_H
#define EXECUTOR_H

int execute_shell_cmd(ShellCmdNode *shell_cmd, char **pwd, char *shell_dir, char *command);

int execute_cmd_group(CmdGroupNode *cmd_group, char **pwd, char *shell_dir, int is_foreground);

int execute_atomic(AtomicNode *atomic, char **pwd, char *shell_dir, int is_foreground);

#endif