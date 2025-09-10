#include "../include/general.h"
#include "../include/background.h"

void sigint_handler(int sig_num){
   //printf("Ctrl-C received");
    if(fg_pgid > 0){
        kill(-fg_pgid, SIGINT);
        sigint_cont = 1;
        fg_pgid = 0;
    }
    printf("\n");
}

void sigtstp_handler(int sig_num){
    //printf("Ctrl-Z received\n");
    if(fg_pgid > 0){
        kill(-fg_pgid, SIGTSTP);
        add_background(fg_pgid, fg_cmd, STOPPED);
        printf("[%d] Stopped %s", job_number, fg_cmd);
        job_number++;
        sigtstp_cont = 1;
        fg_pgid = 0;
    }
    printf("\n");
}