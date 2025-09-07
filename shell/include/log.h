#ifndef LOG_H
#define LOG_H

int pow_ten(int pow);

int str_to_int(char *str);

int valid_log_num(char *str);

int log_verify(AtomicNode *atomic_cmd);

int count_logs();

int check_repeat(char *command, char *log_path);

void log_append(char *commmand, ShellCmdNode *shell_cmd, char *shell_dir);

int execute_log(AtomicNode *atomic_cmd, char *shell_dir, char **pwd, int *job_number);

#endif