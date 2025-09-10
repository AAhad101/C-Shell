#include "../include/general.h"
#include "../include/prompt.h"
#include "../include/parser.h"
#include "../include/hop.h"
#include "../include/executor.h"
#include "../include/log.h"
#include "../include/background.h"
#include "../include/signals.h"

pid_t fg_pgid = 0;
int cont = 0;

int main(){
    char *prev_wd = (char *)malloc(sizeof(char) * PATH_MAX);       // Buffer to store path of previous working directory, empty is none
    char *shell_dir = (char *)malloc(sizeof(char) * PATH_MAX);     // Storing path of the directory of the shell
    getcwd(shell_dir, PATH_MAX);

    // Creates log.txt
    char *log_path = (char *)malloc(sizeof(char) * CMD_MAX);
    strcpy(log_path, shell_dir);
    strcat(log_path, "/log.txt");
    FILE *log_file = fopen(log_path, "a");
    fclose(log_file);

    BG_process *bg_prcs = NULL;
    int active_bgs = 0;
    int job_number = 1;

    while(1){
        signal(SIGINT, sigint_handler);     // Installing signal handler for Ctrl-C
        if(cont){
            cont = 0;
            continue;
        }

        char *command = (char *)malloc(sizeof(char) * CMD_MAX);     // Storing command
        show_prompt(shell_dir);
    
        int ret = scanf(" %[^\n]", command);      
        if(ret == EOF){
            if(errno == EINTR){
                free(command);
                continue;
            }
            for(int i = 0; i < active_bgs; i++){
                kill(bg_prcs[i].pid, SIGKILL);
            }
            printf("logout\n");
            exit(0);
        }

        print_terminated_bg(&bg_prcs, &active_bgs);

        Token *tokenised_cmd = tokenise(command);       // Tokenising command
        
        if(!tokenised_cmd){     // If tokenisation fails
            printf("Invalid Syntax!\n");
        }

        else{
            /*
            char *enum_translate[8] = {"NAME", "PIPE", "AND", "SEMICOLON", "INPUT", "OUTPUT", "APPEND", "EOF"}; 

            int i = 0;
            while(tokenised_cmd[i].type != TOKEN_EOF){
                printf("Token type: %s\n", enum_translate[tokenised_cmd[i].type]);
                if(kenised_cmd[i].type == TOKEN_NAME)
                    printf("Token string: %s\n", tokenised_cmd[i].token);
                printf("Token position: %d\n", tokenised_cmd[i].position);

                printf("\n");
                i++;
            }
            */
    
        ShellCmdNode *root = parse_shell_cmd(&tokenised_cmd);   // Parssing and creating abstract syntax tree
            if(!root){
                printf("Invalid Syntax!\n");
                continue;
            }
            
            execute_shell_cmd(root, &prev_wd, shell_dir, command, &job_number, &bg_prcs, &active_bgs);   // Executing valid command
            log_append(command, root, shell_dir);  // Logging appropriately
        }

        free(command);
    }

    return 0;
}