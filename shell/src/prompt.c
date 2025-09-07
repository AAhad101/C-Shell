#include "general.h"
#include "prompt.h"

void show_prompt(char *shell_dir){
    int username_len;
    int sysname_len;
    int shelldir_len;

    // Getting username
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    if(!pw){
        exit(1);//       
    }

    username_len = strlen(pw->pw_name);        
    char *username = (char *)malloc(sizeof(char) * (username_len + 1));
    strcpy(username, pw->pw_name);

    //printf("User name: %s\n", username);


    // Getting system name
    struct utsname buf;
    if(uname(&buf) != 0){
        exit(1);//      
    }

    sysname_len = strlen(buf.nodename);
    char *sysname = (char *)malloc(sizeof(char) * (sysname_len + 1));
    strcpy(sysname, buf.nodename);

    //printf("System name: %s\n", sysname);

    // Getting path of current and shell home directories
    shelldir_len = strlen(shell_dir);
    char *shell_home_path = (char *)malloc(sizeof(char) * (shelldir_len + 1));
    strcpy(shell_home_path, shell_dir);

    char *cur_path = (char *)malloc(sizeof(char) * (PATH_MAX + 1));
    if(!getcwd(cur_path, PATH_MAX + 1)){
        exit(1);//
    }

    //printf("Current path: %s\n", cur_path);

    // Modifying the current path if it has the shell home directory as an ancestor
    if(strncmp(shell_home_path, cur_path, shelldir_len) == 0){
        int new_path_len = strlen(cur_path) - shelldir_len + 1;
        char *new_path = (char *)malloc(sizeof(char) * (new_path_len + 1));
        new_path[0] = '~';

        for(int i = shelldir_len, j = 1; i < (int)strlen(cur_path); i++, j++){
            new_path[j] = cur_path[i];
        } 
        new_path[new_path_len] = '\0';

        //printf("Updated path: %s\n", new_path);
       
        printf("<%s@%s:%s> ", username, sysname, new_path);
        return;
    } 

    // Printing the final prompt
    printf("<%s@%s:%s> ", username, sysname, cur_path);
}
