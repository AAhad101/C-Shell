#include "../include/general.h"
#include "../include/hop.h"
#include "../include/reveal.h"
#include "../include/parser.h"
#include "../include/arbitrary.h"
#include "../include/log.h"
#include "../include/background.h"
#include "../include/activities.h"
#include "../include/signals.h"
#include "../include/bg.h"
#include "../include/ping.h"

int execute_atomic(AtomicNode *atomic, char **pwd, char *shell_dir, int is_foreground, int is_piped) {
    char *input_file = NULL;
    char *output_file = NULL;
    int append_mode = 0; // 0 for truncate (>), 1 for append (>>)

    int saved_stdin = -1;
    int saved_stdout = -1;
    int ret_value = 0;

    // The command fails if any redirection is invalid.
    if (atomic->count) {
        for (int i = 0; i < atomic->count; i++) {
            char *op = atomic->op[i];
            char *file = atomic->files[i];

            if (strcmp(op, "<") == 0) {
                // Check for read access. If it fails, the whole command fails.
                if (access(file, R_OK) != 0) {
                    perror(file);
                    // Free any allocated memory before returning
                    if (input_file) free(input_file);
                    if (output_file) free(output_file);
                    return 1;
                }
                if (input_file) free(input_file);
                input_file = strdup(file);
            } else if (strcmp(op, ">") == 0 || strcmp(op, ">>") == 0) {
                // Check for write access by attempting to open the file.
                int flags = O_WRONLY | O_CREAT;
                if (strcmp(op, ">>") == 0) {
                    flags |= O_APPEND;
                } else {
                    flags |= O_TRUNC;
                }

                int fd = open(file, flags, 0644);
                if (fd < 0) {
                    printf("Unable to create file for writing\n");
                    if (input_file) free(input_file);
                    if (output_file) free(output_file);
                    return 1;
                }
                close(fd); // Successfully checked, so close it.

                if (output_file) free(output_file);
                output_file = strdup(file);
                append_mode = (strcmp(op, ">>") == 0);
            }
        }
    }

    if (input_file) {
        saved_stdin = dup(STDIN_FILENO);
        int fd_in = open(input_file, O_RDONLY);
        if (fd_in < 0) {
            perror(input_file);
            if (input_file) free(input_file);
            if (output_file) free(output_file);
            return 1;
        }
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
    }

    if (output_file) {
        saved_stdout = dup(STDOUT_FILENO);
        int flags = O_WRONLY | O_CREAT;
        flags |= (append_mode) ? O_APPEND : O_TRUNC;
        
        int fd_out = open(output_file, flags, 0644);
        if (fd_out < 0) {
            perror(output_file);
            if (saved_stdin != -1) {
                dup2(saved_stdin, STDIN_FILENO);
                close(saved_stdin);
            }
            if (input_file) free(input_file);
            if (output_file) free(output_file);
            return 1;
        }
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }

    char* cmd = atomic->argv[0];
    if (strcmp(cmd, "hop") == 0) {
        ret_value = execute_hop(atomic, pwd, shell_dir);
    } else if (strcmp(cmd, "reveal") == 0) {
        ret_value = execute_reveal(atomic, pwd, shell_dir);
    } else if (strcmp(cmd, "log") == 0) {
        ret_value = execute_log(atomic, shell_dir, pwd);
    } else if (strcmp(cmd, "activities") == 0) {
        ret_value = execute_activities(atomic);
    } else if (strcmp(cmd, "ping") == 0) {
        ret_value = execute_ping(atomic);
    } else if (strcmp(cmd, "bg") == 0) {
        ret_value = execute_bg(atomic);
    } else {
        ret_value = execute_arbitrary(atomic, is_foreground, is_piped);
    }

    if (saved_stdin != -1) {
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);
    }
    if (saved_stdout != -1) {
        fflush(stdout);
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }

    if (input_file) free(input_file);
    if (output_file) free(output_file);

    return ret_value;
}

int execute_cmd_group(CmdGroupNode *cmd_group, char **pwd, char *shell_dir, int is_foreground){
    int atomic_num = cmd_group->count;
    if(atomic_num == 0) return 1;

    // If no pipes are there, just execute normally
    if(atomic_num == 1){
        return execute_atomic(cmd_group->atomics[0], pwd, shell_dir, is_foreground, 0);    // 0 because it isn't piped
    }

    int pipes[atomic_num - 1][2];       // Two file descriptors per pipe

    // Creating the required number of pipes (n commands, n-1 pipes)
    for(int i = 0; i < atomic_num - 1; i++){
        if(pipe(pipes[i]) < 0){
            return 1;
        }
    }

    pid_t child_pids[atomic_num];   // Storing PIDs of forked child processes
    pid_t final_pid;

    pid_t pgid = 0;

    // Creating a child process for each pipe and redirecting as required
    for(int i = 0; i < atomic_num; i++){
        pid_t pid = fork();
        if(pid < 0){
            return 1;
        }
        if(pid == 0){
            if(i == 0){     // The first process becomes the leader of the process group
                pgid = getpid();
            }
            setpgid(0, pgid);   // Each process is being assigned the same process group ID

            if(i > 0) dup2(pipes[i-1][0], STDIN_FILENO);    // Other than first command, redirect STDIN to previous pipe's read end
            if(i < atomic_num - 1) dup2(pipes[i][1], STDOUT_FILENO);    // Other than last command, redirect STDOUT to current pipe's write end
            
            // Close all pipe file descriptors in the child
            for(int j = 0; j < atomic_num - 1; j++){
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Executes the atomic
            int status = execute_atomic(cmd_group->atomics[i], pwd, shell_dir, is_foreground, 1);      // 1 because it is piped
            exit(status);
        }
        if(i == 0) pgid = pid;
        setpgid(pid, pgid);
        if(is_foreground){
            fg_pgid = pgid;
            fg_cmd = strdup(stringify_cmd_group(cmd_group));
        }

        child_pids[i] = pid;
        if(i == atomic_num - 1) final_pid = pid;
    }
    
    // Parent closes all pipe file descriptors as the children processes manage them
    for(int i = 0; i < atomic_num - 1; i++){
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    int status;
    int ret_value = 1;
    
    // Parent waits for all children
    for(int i = 0; i < atomic_num; i++){
        pid_t wpid = waitpid(child_pids[i], &status, 0);
        
        if(sigtstp_cont) return 0;

        if(wpid == final_pid){
            if(WIFEXITED(status) && WEXITSTATUS(status) == 0) ret_value = 0;
            else ret_value = 1;
        }
    }

    return ret_value;
}

/*void execute_shell_cmd(ShellCmdNode *shell_cmd, char **pwd, char *shell_dir){
    for(int i = 0; i < shell_cmd->count; i++){
        execute_cmd_group(shell_cmd->cmd_groups[i], pwd, shell_dir);           // HAVE TO LOOK AT SEQUENTIAL AND BACKGROUND
        
        CmdGroupNode *current_cmd_group = shell_cmd->cmd_groups[i];
        for(int j = 0; j < current_cmd_group->count; j++){
            AtomicNode *current_atomic = current_cmd_group->atomics[j];
            for(int k = 0; k < current_atomic->argc; k++){ 
                printf("%s ", current_atomic->argv[k]);
            }
            printf("\n");
        }
    }
    
    return;
}*/

int execute_shell_cmd(ShellCmdNode *shell_cmd, char **pwd, char *shell_dir, char *command){
    // First identify if a shell_group is to be a background process or not
    for(int i = 0; i < shell_cmd->count; i++){
        signal(SIGINT, sigint_handler);     // Installing signal handler for Ctrl-C
        signal(SIGTSTP, sigtstp_handler);   // Installing signal handler for Ctrl-Z

        char op;
        if(i < shell_cmd->count - 1){
            op = shell_cmd->operators[i];
        }
        else if(i == shell_cmd->count - 1){
            if(shell_cmd->background == 1) op = '&';
            else if(shell_cmd->background == 0) op = ';';
        }

        // If it isn't a background process, execute
        if(op == ';'){
            execute_cmd_group(shell_cmd->cmd_groups[i], pwd, shell_dir, 1);    // 1 because it is a foreground process
            fg_pgid = 0;
        }

        else{
            pid_t pid = fork();

            if(pid < 0){
                return 1;
            }
            else if(pid == 0){
                int exit_status = execute_cmd_group(shell_cmd->cmd_groups[i], pwd, shell_dir, 0);
                exit(exit_status);
            }
            else{
                char *cur_cmd_group = stringify_cmd_group(shell_cmd->cmd_groups[i]);
                //printf("Current Command Group: %s\n", cur_cmd_group); //////////////////////
                add_background(pid, cur_cmd_group, RUNNING);
                printf("[%d] %d\n", job_number, pid);
                job_number++;
            }
        }
    }
    return 0; 
}
