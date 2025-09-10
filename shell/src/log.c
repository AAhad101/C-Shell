#include "../include/general.h"
#include "../include/parser.h"
#include "../include/executor.h"

// Function to check if the argument of log execute is valid
int valid_log_num(char *str){
    for(int i = 0; i < (int)strlen(str); i++){
        if(str[i] < '0' || str[i] > '9') return 0;
    }
    int val = str_to_int(str);
    return val >= 1 && val <= 15; 
}

// Function to verify the syntax of inputted log command
int log_verify(AtomicNode *atomic_cmd){
    if(!atomic_cmd->argc || strcmp(atomic_cmd->argv[0], "log") != 0) return 0;      // Return 0 if the atomic_cmd is empty or log isn't the first word
    if(atomic_cmd->argc == 1) return 1;     // Return 1 if the command is just log
    if(atomic_cmd->argc == 2 && strcmp(atomic_cmd->argv[1], "purge") != 0) return 0;    // Return 0 if there's 2 arguments and the second isn't purge
    if(atomic_cmd->argc == 3 && strcmp(atomic_cmd->argv[1], "execute") != 0) return 0;  // Return 0 if there's 3 arguments and the second isn't execute
    if(atomic_cmd->argc == 3 && strcmp(atomic_cmd->argv[1], "execute") == 0 && !valid_log_num(atomic_cmd->argv[2])) return 0;    // Number argument is not valid
    if(atomic_cmd->argc > 3) return 0;      // Return 0 if there's more than 3 arguments
    return 1;
}

// Function to count number of lines existing in log.txt
int count_logs(char *log_path){
    FILE *log_file = fopen(log_path, "r");
    int lines = 0;
    char c;
    while((c = getc(log_file)) != EOF){
        if(c == '\n') lines++;
    }
    fclose(log_file);
    return lines;
}

// Function to check if the current command is the same as the latest one in the log file
int check_repeat(char *command, char *log_path){
    FILE *log_file = fopen(log_path, "r");
    char last_line[CMD_MAX];
    while(fgets(last_line, sizeof(last_line), log_file) != NULL){}
    fclose(log_file);

    last_line[strcspn(last_line, "\n")] = 0;
    if(strcmp(last_line, command) == 0) return 1;
    return 0;
}

void log_append(char *command, ShellCmdNode *shell_cmd, char *shell_dir){
    // Checking if the command entered by the user already has a atomic with the log command or not
    int has_log = 0;
    for(int i = 0; i < shell_cmd->count; i++){
        CmdGroupNode *cur_cmd_group = shell_cmd->cmd_groups[i];
        for(int j = 0; j < cur_cmd_group->count; j++){
            AtomicNode *cur_atomic = cur_cmd_group->atomics[j];
            if(cur_atomic->argc && strcmp(cur_atomic->argv[0], "log") == 0){
                has_log = 1;
            }
        }
    }

    // Storing path of log.txt
    char *log_path = (char *)malloc(sizeof(char) * CMD_MAX);
    strcpy(log_path, shell_dir);
    strcat(log_path, "/log.txt");
    FILE *log_file;

    if(has_log) return;     // We ignore any command with an atomic containing a log command

    int cmds = count_logs(log_path);    // Counts the number of lines in the log file

    // If the current command is the same as the most recent one in the log file, ignore
    if(check_repeat(command, log_path)) return;

    // Else if there are fewer than 15 lines, just append
    if(cmds < 15){
        log_file = fopen(log_path, "a"); 
        fprintf(log_file, "%s\n", command);
        fclose(log_file);

        return;
    }

    // Else, replace the oldest log with the newest one
    else{
        log_file = fopen(log_path, "r");
        char *temp_path = (char *)malloc(sizeof(char) * CMD_MAX);
        strcpy(temp_path, shell_dir);
        strcat(temp_path, "/temp.txt");
        FILE *temp_file = fopen(temp_path, "w");

        char buffer[CMD_MAX];
        int line_count = 0;
        while(fgets(buffer, sizeof(buffer), log_file)){
            line_count++;
            if(line_count == 1) continue;
            fputs(buffer, temp_file);
        }

        fclose(log_file);
        fclose(temp_file);

        rename(temp_path, log_path);
    }
}

int execute_log(AtomicNode *atomic_cmd, char *shell_dir, char **pwd, int *job_number){
    // Checking if the log syntax is valid or not
    int valid_log = log_verify(atomic_cmd);

    if(valid_log == 0){
        //printf("log: Invalid Syntax!\n");     // REMOVED FOR NOW
        return 1;
    }

    int ret_value = 1;
    
    char *log_path = (char *)malloc(sizeof(char) * CMD_MAX);
    strcpy(log_path, shell_dir);
    strcat(log_path, "/log.txt");

    // Executing log
    if(atomic_cmd->argc == 1){
        FILE *log_file = fopen(log_path, "r");
        char line[CMD_MAX];
        while(fgets(line, sizeof(line), log_file) != NULL){
            printf("%s", line);
        }
        fclose(log_file);
        return 0;
    }

    // Executing log purge
    else if(atomic_cmd->argc == 2){
        FILE *log_file = fopen(log_path, "w");
        fclose(log_file);
        return 0;
    }

    // Executing log execute ___
    else if(atomic_cmd->argc == 3){
        int line_num = str_to_int(atomic_cmd->argv[2]);   // Extracting the line number to execute
        int total_lines = count_logs(log_path);             
        if(line_num > total_lines) return 1;              // Returning 1 if argument is greater than number of lines in log file
        line_num = total_lines - line_num + 1;
    
        FILE *log_file = fopen(log_path, "r");
        char command[CMD_MAX];
        int i = 0;
        while(fgets(command, sizeof(command), log_file) != NULL){
            i++;
            if(i == line_num) break;
        }

        command[strcspn(command, "\n")] = 0;     // Command has a \n at the end which must be removed

        // Actually executing the command by tokenising, parsing, executing and logging
        Token *tokenised_cmd = tokenise(command);   // Tokenising
        if(!tokenised_cmd){     // If tokenisation fails
            printf("Invalid Syntax!\n");
            return 1;
        }

        else{
            ShellCmdNode *root = parse_shell_cmd(&tokenised_cmd);   // Parssing and creating abstract syntax tree
            if(!root){
                printf("Invalid Syntax!\n");
                return 1;
            }
            
            ret_value = execute_shell_cmd(root, pwd, shell_dir, command);   // Executing valid command
            log_append(command, root, shell_dir);  // Logging appropriately
        }

        return ret_value;
    }

    return ret_value;
}
