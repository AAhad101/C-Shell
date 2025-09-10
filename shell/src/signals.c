#include "../include/general.h"

void sigint_handler(int sig_num){
   //printf("Ctrl-C received");
    if(fg_pgid > 0){
        kill(-fg_pgid, SIGINT);
        cont = 1;
        fg_pgid = 0;
    }
    printf("\n");
    //fflush(stdout);
}
