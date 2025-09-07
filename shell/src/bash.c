#include "general.h"
#include "parser.h"

int execute_bash(AtomicNode *atomic_cmd){
    char *args[4];
    for(int i = 0; i < 4; i++){
        args[i] = (char *)malloc(sizeof(char) * CMD_MAX);
    }

    args[0] = "bash";
    args[1] = "-c";
    args[3] = NULL;

    char *command_string = (char *)malloc(sizeof(char) * CMD_MAX);

    strcpy(command_string, atomic_cmd->argv[0]);
    strcat(command_string, " ");

    for(int i = 1; i < atomic_cmd->argc; i++){
        strcat(command_string, atomic_cmd->argv[i]);
        strcat(command_string, " ");
    }

    for(int i = 0; i < atomic_cmd->count; i++){
        char *op = atomic_cmd->op[i];
        char *file_name = atomic_cmd->files[i];

        strcat(command_string, op);
        strcat(command_string, " ");
        strcat(command_string, file_name);
        strcat(command_string, " ");
    }

    args[2] = command_string;
    //signal(SIGINT, SIG_IGN);

    pid_t pid = fork();

    if(pid < 0) return 1;

    else if(pid == 0){
        //signal(SIGINT, SIG_DFL);
        execvp(args[0], args);
        exit(1);
    }

    else{
        int status;
        waitpid(pid, &status, 0);
        if(WIFEXITED(status) && WEXITSTATUS(status) == 0) return 0;
        else return 1;
    }
}