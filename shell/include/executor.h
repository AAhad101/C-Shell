#ifndef EXECUTOR_H
#define EXECUTOR_H

void execute_shell_cmd(ShellCmdNode *shell_cmd, char **pwd, char *shell_dir);

void execute_shell_cmd(ShellCmdNode *shell_cmd, char **pwd, char *shell_dir);

void execute_atomic(AtomicNode *atomic, char **pwd, char *shell_dir);

#endif