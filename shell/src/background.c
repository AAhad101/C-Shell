#include "../include/general.h"
#include "../include/parser.h"

void print_bgs(BG_process *bg_prcs, int active_bgs){
    for(int i = 0; i < active_bgs; i++){
        BG_process cur_prcs = bg_prcs[i];
        printf("%d %d %s %d\n", cur_prcs.pid, cur_prcs.job_no, cur_prcs.command, cur_prcs.status);
    }
}

void add_background(pid_t pid, int job_no, char *command, BG_process **bg_prcs, int *active_bgs){
    if(*active_bgs == 0){
        (*active_bgs)++;
        *bg_prcs = (BG_process *)malloc(sizeof(BG_process));
        BG_process cur_prcs;
        cur_prcs.pid = pid;
        cur_prcs.job_no = job_no;
        cur_prcs.status = RUNNING;
        cur_prcs.command = strdup(command);
        //printf("Command: %s\n", cur_prcs.command);
        (*bg_prcs)[*active_bgs - 1] = cur_prcs;

        return;
    }
 
    (*active_bgs)++;
    *bg_prcs = (BG_process *)realloc(*bg_prcs, sizeof(BG_process) * (*active_bgs));
    BG_process cur_prcs;
    cur_prcs.pid = pid;
    cur_prcs.job_no = job_no;
    cur_prcs.status = RUNNING;
    cur_prcs.command = strdup(command);
    //printf("Command: %s\n", cur_prcs.command);    
    (*bg_prcs)[*active_bgs - 1] = cur_prcs;

    return;
}

void remove_terminated_bg(BG_process **bg_prcs, int *active_bgs){
    int not_terminated = 0;
    for(int i = 0; i < *active_bgs; i++){
        BG_process cur_prcs = (*bg_prcs)[i];
        if(cur_prcs.status != TERMINATED){
            not_terminated++;
        } 
    }

    BG_process *new_bg_prcs = (BG_process *)malloc(sizeof(BG_process) * not_terminated);
    int k = 0;

    for(int i = 0; i < *active_bgs; i++){
        BG_process cur_prcs = (*bg_prcs)[i];
        if(cur_prcs.status != TERMINATED){
            new_bg_prcs[k++] = cur_prcs;
        }
    }

    BG_process *temp = *bg_prcs;
    *bg_prcs = new_bg_prcs;
    
    for(int i = 0; i < *active_bgs; i++){
        free(temp[i].command);
    }
    free(temp);

    *active_bgs = k;
    
    return;
}

void print_terminated_bg(BG_process **bg_prcs, int *active_bgs){
    for(int i = 0; i < *active_bgs; i++){
        BG_process cur_prcs = (*bg_prcs)[i];
        pid_t pid = cur_prcs.pid;
        int status;
        waitpid(pid, &status, WNOHANG);

        if(WIFEXITED(status) && WEXITSTATUS(status) == 0){
            printf("%s with pid %d exited normally\n", cur_prcs.command, cur_prcs.pid);
            ((*bg_prcs)[i]).status = TERMINATED;
        }
        else if(WIFEXITED(status) && WEXITSTATUS(status) == 1){
            printf("%s with pid %d exited abnormally\n", cur_prcs.command, cur_prcs.pid);
            ((*bg_prcs)[i]).status = TERMINATED;   
        }
    }

    remove_terminated_bg(bg_prcs, active_bgs);
}