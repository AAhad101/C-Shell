#include "../include/general.h"
#include "../include/hop.h"
#include "../include/reveal.h"
#include "../include/parser.h"
#include "../include/bash.h"
#include "../include/log.h"
#include "../include/background.h"
#include "../include/activities.h"

int execute_atomic(AtomicNode *atomic, char **pwd, char *shell_dir, int *job_number, BG_process **bg_prcs, int *active_bgs){
    char *last_in = (char *)malloc(sizeof(char) * CMD_MAX);
    char *last_outapp = (char *)malloc(sizeof(char) * CMD_MAX);
    int in_flag = 0;
    int out_flag = 0;
    int app_flag = 0;
  
    if(atomic->count){
        for(int i = 0; i < atomic->count; i++){
            char *op = atomic->op[i];
            char *file = atomic->files[i];
            if(strcmp(op, "<") == 0){
                in_flag = 1;
                strcpy(last_in, file);
            }
            else if(strcmp(op, ">") == 0){
                out_flag = 1;
                app_flag = 0;
                strcpy(last_outapp, file);
            }
            else if(strcmp(op, ">>") == 0){
                app_flag = 1;
                out_flag = 0;
                strcpy(last_outapp, file);
            }
        }
    }

    if(in_flag){
        FILE *input_file = fopen(last_in, "r");
        if(!input_file){
            printf("No such file or directory\n");
            free(last_in);
            free(last_outapp);
            return 1;
        }
        fclose(input_file);
    }

    int ret_value = 1;

    if(strcmp(atomic->argv[0], "hop") == 0){        /// CHANGE CHANGE CHANGE CHANGE 
        if(out_flag){
            int saved_stdout = dup(STDOUT_FILENO);
            int fd = open(last_outapp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);

            ret_value = execute_hop(atomic, pwd, shell_dir);

            fflush(stdout);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
        }
        else if(app_flag){
            int saved_stdout = dup(STDOUT_FILENO);
            int fd = open(last_outapp, O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);

            ret_value = execute_hop(atomic, pwd, shell_dir);

            fflush(stdout);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);   
        }
        else{
            ret_value = execute_hop(atomic, pwd, shell_dir);
        }
    }
    else if(strcmp(atomic->argv[0], "reveal") == 0){
        if(out_flag){
            int saved_stdout = dup(STDOUT_FILENO);
            int fd = open(last_outapp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);

            ret_value = execute_reveal(atomic, pwd, shell_dir);

            fflush(stdout);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
        }
        else if(app_flag){
            int saved_stdout = dup(STDOUT_FILENO);
            int fd = open(last_outapp, O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);

            ret_value = execute_reveal(atomic, pwd, shell_dir);

            fflush(stdout);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);   
        }
        else{
            ret_value = execute_reveal(atomic, pwd, shell_dir);
        }
    }
    else if(strcmp(atomic->argv[0], "log") == 0){
        if(out_flag){
            int saved_stdout = dup(STDOUT_FILENO);
            int fd = open(last_outapp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);

            ret_value = execute_log(atomic, shell_dir, pwd, job_number, bg_prcs, active_bgs);

            fflush(stdout);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
        }
        else if(app_flag){
            int saved_stdout = dup(STDOUT_FILENO);
            int fd = open(last_outapp, O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);

            ret_value = execute_log(atomic, shell_dir, pwd, job_number, bg_prcs, active_bgs);

            fflush(stdout);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);   
        }
        else{
            ret_value = execute_log(atomic, shell_dir, pwd, job_number, bg_prcs, active_bgs);
        }
    }
    else if(strcmp(atomic->argv[0], "activities") == 0){
        if(out_flag){
            int saved_stdout = dup(STDOUT_FILENO);
            int fd = open(last_outapp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);

            ret_value = execute_activities(atomic, bg_prcs, active_bgs);

            fflush(stdout);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
        }
        else if(app_flag){
            int saved_stdout = dup(STDOUT_FILENO);
            int fd = open(last_outapp, O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);

            ret_value = execute_activities(atomic, bg_prcs, active_bgs);

            fflush(stdout);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);   
        }
        else{
            ret_value = execute_activities(atomic, bg_prcs, active_bgs);
        }
    }
    else{
        ret_value = execute_bash(atomic);
    }

    free(last_in);
    free(last_outapp);
    return ret_value;
}

int execute_cmd_group(CmdGroupNode *cmd_group, char **pwd, char *shell_dir, int *job_number, BG_process **bg_prcs, int *active_bgs){
    int atomic_num = cmd_group->count;
    if(atomic_num == 0) return 1;

    // If no pipes are there, just execute normally
    if(atomic_num == 1){
        return execute_atomic(cmd_group->atomics[0], pwd, shell_dir, job_number, bg_prcs, active_bgs);
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

    // Creating a child process for each pipe and redirecting as required
    for(int i = 0; i < atomic_num; i++){
        pid_t pid = fork();
        if(pid < 0){
            return 1;
        }
        if(pid == 0){
            if(i > 0) dup2(pipes[i-1][0], STDIN_FILENO);    // Other than first command, redirect STDIN to previous pipe's read end
            if(i < atomic_num - 1) dup2(pipes[i][1], STDOUT_FILENO);    // Other than last command, redirect STDOUT to current pipe's write end
            
            // Close all pipe file descriptors in the child
            for(int j = 0; j < atomic_num - 1; j++){
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Executes the atomic
            int status = execute_atomic(cmd_group->atomics[i], pwd, shell_dir, job_number, bg_prcs, active_bgs);
            exit(status);
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

int execute_shell_cmd(ShellCmdNode *shell_cmd, char **pwd, char *shell_dir, char *command, int *job_number, BG_process **bg_prcs, int *active_bgs){
    // First identify if a shell_group is to be a background process or not
    for(int i = 0; i < shell_cmd->count; i++){
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
            return execute_cmd_group(shell_cmd->cmd_groups[i], pwd, shell_dir, job_number, bg_prcs, active_bgs);
        }

        else{
            pid_t pid = fork();

            if(pid < 0){
                return 1;
            }
            else if(pid == 0){
                int exit_status = execute_cmd_group(shell_cmd->cmd_groups[i], pwd, shell_dir, job_number, bg_prcs, active_bgs);
                exit(exit_status);
            }
            else{
                int cur_jobno = *job_number;
                (*job_number)++;
                printf("[%d] %d\n", cur_jobno, pid);
                char *cur_cmd_group = stringify_cmd_group(shell_cmd->cmd_groups[i]);
                //printf("Current Command Group: %s\n", cur_cmd_group); //////////////////////
                add_background(pid, cur_jobno, cur_cmd_group, bg_prcs, active_bgs);
            }
        }
    }
    return 0; 
}