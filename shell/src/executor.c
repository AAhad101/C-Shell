#include "general.h"
#include "hop.h"
#include "reveal.h"
#include "parser.h"
#include "bash.h"
#include "log.h"

void execute_atomic(AtomicNode *atomic, char **pwd, char *shell_dir){
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
            return;
        }
        fclose(input_file);
    }

    if(strcmp(atomic->argv[0], "hop") == 0){
        execute_hop(atomic, pwd, shell_dir);
        if(out_flag || app_flag){
            FILE *file = fopen(last_outapp, "w");
            fclose(file);
        }
    }
    else if(strcmp(atomic->argv[0], "reveal") == 0){
        if(out_flag){
            int saved_stdout = dup(STDOUT_FILENO);
            int fd = open(last_outapp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);

            execute_reveal(atomic, pwd, shell_dir);

            fflush(stdout);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
        }
        else if(app_flag){
            int saved_stdout = dup(STDOUT_FILENO);
            int fd = open(last_outapp, O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);

            execute_reveal(atomic, pwd, shell_dir);

            fflush(stdout);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);   
        }
        else{
            execute_reveal(atomic, pwd, shell_dir);
        }
    }
    else if(strcmp(atomic->argv[0], "log") == 0){
        if(out_flag){
            int saved_stdout = dup(STDOUT_FILENO);
            int fd = open(last_outapp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);

            execute_log(atomic, shell_dir, pwd);

            fflush(stdout);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
        }
        else if(app_flag){
            int saved_stdout = dup(STDOUT_FILENO);
            int fd = open(last_outapp, O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);

            execute_log(atomic, shell_dir, pwd);

            fflush(stdout);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);   
        }
        else{
            execute_log(atomic, shell_dir, pwd);
        }
    }
    else{
        execute_bash(atomic);
    }

    free(last_in);
    free(last_outapp);
    return;
}

void execute_cmd_group(CmdGroupNode *cmd_group, char **pwd, char *shell_dir){
    for(int i = 0; i < cmd_group->count; i++){
        execute_atomic(cmd_group->atomics[i], pwd, shell_dir);          // HAVE TO LOOK AT PIPING
    }

    return;
}

void execute_shell_cmd(ShellCmdNode *shell_cmd, char **pwd, char *shell_dir){
    for(int i = 0; i < shell_cmd->count; i++){
        execute_cmd_group(shell_cmd->cmd_groups[i], pwd, shell_dir);           // HAVE TO LOOK AT SEQUENTIAL AND BACKGROUND
    }

    return;
}
