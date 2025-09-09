#include "../include/general.h"
#include "../include/hop.h"

// Function to validate syntax of hop command
int hop_verify(AtomicNode *atomic_cmd){
    if(!atomic_cmd->argc || strcmp(atomic_cmd->argv[0], "hop") != 0) return 0;      // If there's no arguments or the atomic_cmd doesn't start with hop

    if(atomic_cmd->argc == 1) return 1;         // If there's no arguments after hop, return 1

    if(atomic_cmd->count > 0) return 0;       // If there are input or output redirections, or some files

    return 1;       // Else the command will be valid
}

// Function to execute hop
int execute_hop(AtomicNode *atomic_cmd, char **pwd, char *shell_dir){
    int valid_hop = hop_verify(atomic_cmd);
    if(!valid_hop) return 1;        // Return 1 (invalid) if hop syntax is violated

    char *cur_dir = (char *)malloc(sizeof(char) * PATH_MAX);
    getcwd(cur_dir, PATH_MAX);

    /*printf("Current Directory: %s\n", cur_dir);    
    printf("Shell Directory: %s\n", shell_dir);
    if(*pwd) printf("Previous Working Directory: %s\n", *pwd);*/

    if(atomic_cmd->argc == 1){
        chdir(shell_dir);
        strcpy(*pwd, cur_dir);
        free(cur_dir);
        return 0;       // No arguments means go to home of the shell directory
    }

    for(int i = 1; i < atomic_cmd->argc; i++){
        char *hop_loc = atomic_cmd->argv[i];
        
        char *cur_dir = (char *)malloc(sizeof(char) * PATH_MAX);
        getcwd(cur_dir, PATH_MAX);

        if(strcmp(hop_loc, "~") == 0){
            chdir(shell_dir);
            strcpy(*pwd, cur_dir);
        }
        else if(strcmp(hop_loc, ".") == 0){
            strcpy(*pwd, cur_dir);
            continue;
        }
        else if(strcmp(hop_loc, "..") == 0){
            chdir("..");
            strcpy(*pwd, cur_dir);
        }
        else if(strcmp(hop_loc, "-") == 0){
            if((int)strlen(*pwd)){
                chdir(*pwd);    
                strcpy(*pwd, cur_dir);
            }
        }
        else{
            if(chdir(hop_loc) == 0){
                strcpy(*pwd, cur_dir);
            }

            else{                                
                printf("No such directory!\n");
                free(cur_dir);
                return 1;
            }
        }
   }

   return 0;
}