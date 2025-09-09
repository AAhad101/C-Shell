#include "../include/general.h"
#include "../include/reveal.h"

// Comparator to sort file and directory names by ASCII value
int cmp_strings(const void *name1, const void *name2){
    return strcmp(*(const char **)name1, *(const char **)name2);
}

// Function to validate syntax of reveal command
int reveal_verify(AtomicNode *atomic_cmd){
    if(!atomic_cmd->argc || strcmp(atomic_cmd->argv[0], "reveal") != 0) return 0;

    if(atomic_cmd->argc == 0) return 1;

    int i = 1;
    while(i < atomic_cmd->argc && atomic_cmd->argv[i][0] == '-') i++;       // Read all flag tokens

    if(i < atomic_cmd->argc-1)  return -1;       // If there's more than one token (name) after the flags, invalid syntax

    return 1;       // Valid syntax, so return 1
}

// Function to execute reveal
int execute_reveal(AtomicNode *atomic_cmd, char **pwd, char *shell_dir){
    int valid_reveal = reveal_verify(atomic_cmd);
    if(!valid_reveal) return 1;     // Invalid reveal syntax  

    if(valid_reveal == -1){
        printf("reveal: Invalid Syntax!\n");
        return 1;
    }

    int a_flag = 0;     // Flag to check if a flag has been used
    int l_flag = 0;     // Flag to see if l flag has been used
    char *f_token = (char *)malloc(sizeof(char) * CMD_MAX);

    int i = 1;
    
    while(i < atomic_cmd->argc && atomic_cmd->argv[i][0] == '-' && (int)strlen(atomic_cmd->argv[i]) > 1 && (!a_flag || !l_flag)){       // Checking if a and/or l flags have been set 
        int j = 1;
        char *cur_token = atomic_cmd->argv[i];
        while(j < (int)strlen(cur_token) && (!a_flag || !l_flag)){
            if(cur_token[j] == 'l') l_flag = 1;
            if(cur_token[j] == 'a') a_flag = 1;
            j++;
        }
        i++;
    }

    while(i < atomic_cmd->argc && atomic_cmd->argv[i][0] == '-' && (int)strlen(atomic_cmd->argv[i]) > 1) i++;       // Skipping any remaining flag tokens

    if(i < atomic_cmd->argc){
        f_token = atomic_cmd->argv[i];     // Storing final token, if any
    }
    
    if((int)strlen(f_token) == 0) f_token = ".";
    if(strcmp(f_token, "~") == 0) f_token = shell_dir;       // If there is no final token, we have to print from the home directory relative to shell
    if((int)strlen(*pwd) == 0 && strcmp(f_token, "-") == 0){             // If previous working directory hasn't been set
        printf("No such directory!\n");
        return 1;
    }
    else if(strcmp(f_token, "-") == 0){
        f_token = *pwd;
    }

    // Storing all files and directories in an array then sorting
    DIR *dir = opendir(f_token);
    if(!dir){
        printf("No such directory!\n");
        return 1;
    }

    struct dirent *entry;
    char *names[MAX_SIZE];      // Array to store all file and directory names
    int count = 0;

    while((entry = readdir(dir)) != NULL){
        names[count] = (char *)malloc(sizeof(char) * (strlen(entry->d_name) + 1));
        strcpy(names[count], entry->d_name);
        count++;
    }

    closedir(dir);

    qsort(names, count, sizeof(char *), cmp_strings);       // Sorting array of file and directory names

    if(a_flag && l_flag){
        for(int i = 0; i < count; i++){
            printf("%s\n", names[i]);
            free(names[i]);
        }
    }

    else if(a_flag){
        for(int i = 0; i < count; i++){
            printf("%s  ", names[i]);
            free(names[i]);            
        }
    }

    else if(l_flag){
        for(int i = 0; i < count; i++){
            if(names[i][0] != '.'){
                printf("%s\n", names[i]);   
            }
            free(names[i]);
        }
    }

    else{
        for(int i = 0; i < count; i++){
            if(names[i][0] != '.'){
                printf("%s  ", names[i]);
            }
            free(names[i]);
        }
    }

    if(!l_flag) printf("\n");

    return 0;
}