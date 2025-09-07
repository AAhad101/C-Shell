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

    // Creates log.txt
    char *log_path = (char *)malloc(sizeof(char) * CMD_MAX);
    strcpy(log_path, shell_dir);
    strcat(log_path, "/log.txt");
    FILE *log_file = fopen(log_path, "a");
    fclose(log_file);

    // Creates background.txt
    char *back_path = (char *)malloc(sizeof(char) *CMD_MAX);
    strcpy(back_path, shell_dir);
    strcat(back_path, "/background.txt");
    FILE *back_file = fopen(back_path, "a");
    fclose(back_file);

    int job_number = 1;

    while(1){
        char *command = (char *)malloc(sizeof(char) * CMD_MAX);     // Storing command
        show_prompt(shell_dir);
    
        scanf(" %[^\n]", command);      

        print_completed_bg(shell_dir);

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
            
            execute_shell_cmd(root, &prev_wd, shell_dir, command, &job_number);   // Executing valid command
            log_append(command, root, shell_dir);  // Logging appropriately
        }

        free(command);
    }
    return 0;
}

