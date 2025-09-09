#include "../include/general.h"
#include "../include/parser.h"
#include "../include/background.h"

int compare_bgs(const void *a, const void *b){
    char *bg_a = ((BG_process *)a)->command;
    char *bg_b = ((BG_process *)b)->command;

    return strcmp(bg_a, bg_b);
}

int verify_activities(AtomicNode *atomic){
    if(atomic->argc == 1 && strcmp(atomic->argv[0], "activities") == 0) return 1;
    return 0;
}

int execute_activities(AtomicNode *atomic, BG_process **bg_prcs, int *active_bgs){
    int valid_activities = verify_activities(atomic);
    if(!valid_activities) return 1;

    qsort(*bg_prcs, *active_bgs, sizeof(BG_process), compare_bgs);
    
    for(int i = 0; i < *active_bgs; i++){
        BG_process cur_prcs = (*bg_prcs)[i];
        char *state = (char *)malloc(sizeof(char) * 25);
        if(cur_prcs.status == RUNNING) strcpy(state, "Running");
        else if(cur_prcs.status == STOPPED) strcpy(state, "Stopped");
        else if(cur_prcs.status == TERMINATED){
            free(state);
            continue;
        }
        char *command = (char *)malloc(sizeof(char) * CMD_MAX);
        strcpy(command, cur_prcs.command);

        printf("[%d] : %s - %s\n", cur_prcs.pid, command, state);
        free(state);
        free(command);
    }

    return 0;
}