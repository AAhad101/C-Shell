#include "../include/general.h"
#include "../include/parser.h"

int verify_fg(AtomicNode *atomic_cmd){
    if(strcmp(atomic_cmd->argv[0], "fg") != 0) return 0;

    if(atomic_cmd->argc > 2) return 0;
    
    if(atomic_cmd->argc == 1) return 1;

    if(!is_natural_num(atomic_cmd->argv[1])) return -1;

    return 1;
}

int execute_fg(AtomicNode *atomic_cmd){
    int valid_fg = verify_fg(atomic_cmd);

    if(valid_fg == -1){
        printf("No such job\n");
        return 1;
    }

    if(atomic_cmd->argc == 1){
        if(active_bgs == 0) return 0;

        pid_t pid = 0;
        int max_jobno = 0;
        char *command = 0;

        for(int i = 0; i < active_bgs; i++){
            BG_process cur_prcs = bg_prcs[i];
            if(cur_prcs.job_no > max_jobno){
                pid = cur_prcs.pid;
                max_jobno = cur_prcs.job_no;
                command = strdup(cur_prcs.command);
            }           
        }

        kill(pid, SIGCONT);
        printf("%s\n", command);
        tcsetpgrp(STDIN_FILENO, pid);

        return 0;
    }

    else{
        int given = str_to_int(atomic_cmd->argv[1]);
        int found = 0;
        char *command;

        for(int i = 0; i < active_bgs; i++){
            BG_process cur_prcs = bg_prcs[i];
            if(cur_prcs.pid == given){
                found = 1;
                command = strdup(cur_prcs.command);
            }
        }

        if(!found){
            printf("No such job\n");
            return 1;
        }

        else{
            kill(given, SIGCONT);
            printf("%s\n", command);
            tcsetpgrp(STDIN_FILENO, given);
            return 0;
        }
    }
}
