#include "../include/general.h"
#include "../include/parser.h"

int num_len(int num){
    if(num == 0) return 1;

    int i = 0;
    while(num > 0){
        i++;
        num /= 10;
    }
    return i;
}

char *int_to_str(int number){
    int len = num_len(number);

    char *num = (char *)malloc((len+1)*sizeof(char));

    for(int i = len-1; i >= 0; i--){
        int dig = number % 10;
        num[i] = dig + '0';
        number /= 10;
    }

    num[len] = 0;

    return num;
}

// Function to calculate positive integral powers of 10
int pow_ten(int pow){
    int ans = 1;
    for(int i = 0; i < pow; i++){
        ans *= 10;
    }
    return ans;
}

// Function to convert string to integer
int str_to_int(char *str){
    int len = (int)strlen(str);
    int val = 0;
    for(int i = len-1; i >= 0; i--){
        val += pow_ten(len - i - 1) * (str[i] - '0');
    }
    return val;
}

char *stringify_atomic(AtomicNode *atomic_cmd){
    char *string = (char *)malloc(sizeof(char) * CMD_MAX);

    strcpy(string, atomic_cmd->argv[0]);

    for(int i = 1; i < atomic_cmd->argc; i++){
        strcat(string, " ");
        strcat(string, atomic_cmd->argv[i]);
    }

    for(int i = 0; i < atomic_cmd->count; i++){
        strcat(string, " ");
        char *op = atomic_cmd->op[i];
        char *file_name = atomic_cmd->files[i];

        strcat(string, op);
        strcat(string, " ");
        strcat(string, file_name);
    }

    return string;
}

char *stringify_cmd_group(CmdGroupNode *cmd_group){
    char *string = (char *)malloc(sizeof(char) * CMD_MAX);
    strcpy(string, stringify_atomic(cmd_group->atomics[0]));
    
    for(int i = 1; i < cmd_group->count; i++){
        strcat(string, " | ");
        strcat(string, stringify_atomic(cmd_group->atomics[i]));
    }

    return string;
}