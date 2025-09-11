#include "../include/general.h"
#include "../include/parser.h"

int verify_ping(AtomicNode *atomic){
    if(strcmp(atomic->argv[0], "ping") != 0) return 0;

    if(atomic->argc != 3) return 0;

    if(!is_natural_num(atomic->argv[1])) return -1;  // No such process found 
    
    if(!is_natural_num(atomic->argv[2])) return 0;

    return 1;
}

int execute_ping(AtomicNode * atomic){
    int valid_ping = verify_ping(atomic);

    if(valid_ping == -1){
        printf("No such process found\n");
        return 1;
    }

    int found = 0;
    BG_process req_prcs;
    int index = -1;

    for(int i = 0; i < active_bgs; i++){
        BG_process cur_prcs = bg_prcs[i];
        if(cur_prcs.pid == atoi(atomic->argv[1])){
            req_prcs = cur_prcs;
            found = 1;
            index = i;
            break;
        }
    }

    if(!found){
        printf("No such process found");
        return 1;
    }
    else if(!valid_ping){
        printf("Invalid syntax!\n");
        return 1;
    }

    int pid = req_prcs.pid;
    int sig_num = atoi(atomic->argv[2]) % 32;
    kill(pid, sig_num);
    
    if(sig_num){
        bg_prcs[index].status = TERMINATED;
    }
    
    printf("Sent signal %d to process with pid %d\n", sig_num, pid);


    return 0;
}