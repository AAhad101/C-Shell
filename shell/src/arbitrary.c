#include "../include/general.h"
#include "../include/parser.h"

int execute_arbitrary(AtomicNode *atomic_cmd, int is_foreground, int is_piped){
    pid_t pid = fork();

    if(pid < 0) return 1;

    else if(pid == 0){
        setpgid(0, 0);

        execvp(atomic_cmd->argv[0], atomic_cmd->argv);
        printf("Command not found!\n");
        exit(1);
    }

    else{
        if(is_foreground && !is_piped) fg_pgid = pid;
        //printf("fg_pgid %d\n", fg_pgid);

        int status;
        waitpid(pid, &status, 0);
        if(WIFEXITED(status) && WEXITSTATUS(status) == 0){
            fg_pgid = 0;
            return 0;
        }
        else{
            fg_pgid = 0;
            return 1;
        }
    }
}