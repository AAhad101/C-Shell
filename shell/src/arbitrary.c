#include "../include/general.h"
#include "../include/parser.h"

int execute_arbitrary(AtomicNode *atomic_cmd){
    pid_t pid = fork();

    if(pid < 0) return 1;

    else if(pid == 0){
        execvp(atomic_cmd->argv[0], atomic_cmd->argv);
        printf("Command not found!\n");
        exit(1);
    }

    else{
        int status;
        waitpid(pid, &status, 0);
        if(WIFEXITED(status) && WEXITSTATUS(status) == 0) return 0;
        else return 1;
    }
}