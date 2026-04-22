#include "../include/general.h"
#include "../include/parser.h"
#include "../include/background.h"
#include "../include/signals.h"

int verify_bg(AtomicNode *atomic_cmd){
    if(strcmp(atomic_cmd->argv[0], "fg") != 0) return 0;

    if(atomic_cmd->argc > 2) return 0;
    
    if(atomic_cmd->argc == 1) return 1;

    if(!is_natural_num(atomic_cmd->argv[1])) return -1;

    return 1;
}

int execute_bg(AtomicNode *atomic_cmd){
    int valid_bg = verify_bg(atomic_cmd);

    if(valid_bg == -1){
        printf("No such job\n");
        return 1;
    }

    pid_t pgid = -1;
    char *command;
    int job_no;

    if(atomic_cmd->argc == 1){
        if(active_bgs == 0) return 0;

        pid_t pid = 0;
        int max_jobno = 0;
        
        for(int i = 0; i < active_bgs; i++){
            BG_process cur_prcs = bg_prcs[i];
            if(cur_prcs.job_no > max_jobno){
                pid = cur_prcs.pid;
                max_jobno = cur_prcs.job_no;
                job_no = cur_prcs.job_no;
                command = strdup(cur_prcs.command);
                bg_prcs[i].status = RUNNING;
            }           
        }

        pgid = getpgid(pid);

        kill(-pgid, SIGCONT);
        printf("[%d] %s &\n", job_no, command);
    }

    else{
        int given = str_to_int(atomic_cmd->argv[1]);
        int found = 0;

        for(int i = 0; i < active_bgs; i++){
            BG_process cur_prcs = bg_prcs[i];
            if(cur_prcs.pid == given){
                found = 1;
                command = strdup(cur_prcs.command);
                job_no = bg_prcs[i].job_no;

                if(bg_prcs[i].status == RUNNING){
                    printf("Job already running\n");
                    return 1;
                } 

                bg_prcs[i].status = RUNNING;
            }
        }

        if(!found){
            printf("No such job\n");
            return 1;
        }

        pgid = getpgid(given);
        
        kill(-pgid, SIGCONT);
        printf("[%d] %s &\n", job_no, command);

        return 0;
    }

    return 0;
}