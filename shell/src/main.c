#include "general.h"
#include "prompt.h"
#include "parser.h"
#include "hop.h"
#include "executor.h"
#include "log.h"

int main(){
    char *prev_wd = (char *)malloc(sizeof(char) * PATH_MAX);       // Buffer to store path of previous working directory, empty is none
    char *shell_dir = (char *)malloc(sizeof(char) * PATH_MAX);     // Storing path of the directory of the shell
    getcwd(shell_dir, PATH_MAX);

    while(1){
        char *command = (char *)malloc(sizeof(char) * CMD_MAX);     // Storing command
        show_prompt(shell_dir);
    
        scanf(" %[^\n]", command);      

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
                if(tokenised_cmd[i].type == TOKEN_NAME)
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
            
            execute_shell_cmd(root, &prev_wd, shell_dir);   // Executing valid command
            log_append(command, root, shell_dir);  // Logging appropriately
        }
    }
    return 0;
}


// create variable to store the previous working directory, NULL if none, and pass to execute_hop fuction