#include "../include/general.h"
#include "../include/parser.h"


// Function to identify whitespaces
int is_whitespace(char c){
    return c == ' ' || c == '\r' || c == '\t' || c == '\n';
}

// Function to get the list of tokens
Token *tokenise(char *input){
    Token *tokens = (Token *)malloc(sizeof(Token) * MAX_TOKENS);        // List of tokens
    int token_count = 0;
    int i = 0;
    int name_invalid = 0;       // Flag to check if the command is valid as of now or not
    
    while(i < (int)strlen(input)){    
        char cur = input[i];
        char next = i + 1 < (int)strlen(input) ? input[i+1] : 0;

        if(is_whitespace(cur)){     // Skip all whitespaces
            i++;
        }
        else if(cur == '|'){        // Handling all special characters
            tokens[token_count++] = (Token){TOKEN_PIPE, NULL, i};
            i++;
        }
        else if(cur == '&'){
            tokens[token_count++] = (Token){TOKEN_AND, NULL, i};
            i++;
        }
        else if(cur == ';'){
            tokens[token_count++] = (Token){TOKEN_SEMICOLON, NULL, i};
            i++;
        }
        else if(cur == '<'){
            tokens[token_count++] = (Token){TOKEN_INPUT, NULL, i};
            i++;
        }
        else if(cur == '>'){
            if(next == '>'){
                tokens[token_count++] = (Token){TOKEN_APPEND, NULL, i};
                i += 2;
            }
            else{
                tokens[token_count++] = (Token){TOKEN_OUTPUT, NULL, i}; 
                i++;
            }
        }
        else{                       // Handling 'name', and checking if it is valid
            char *name = (char *)malloc(sizeof(char) * MAX_NAME_LEN);
            int len = 0;
            int quote_flag = 0;
            char quotes = 0;

            while(i + len < (int)strlen(input) && len < MAX_NAME_LEN){
                cur = input[i+len];
                
                if(quote_flag){     
                    if(cur == quotes){
                        quote_flag = 0;
                        name[len++] = cur;
                        break;                      // If we encounter close quotes, name string is terminated, and quote flag is unset
                    }                
                    else{
                        name[len++] = cur;          // Adding everything withing quotes  
                    }
                }

                else{
                    if(cur == '\'' || cur == '"'){
                        quote_flag = 1;             // If we encounter open quotes, quote flag is set and we store the type of quotes in quotes
                        quotes = cur;

                        name[len++] = cur;
                    }                    

                    else if(cur == '|' || cur == '&' || cur == ';' || cur == '<' || cur == '>'){
                        //name_invalid = 1;           // If | or & or ; or < or > are there outside of quotes, the command becomes invalid
                        break;
                    }

                    else if(is_whitespace(cur)){
                        break;                      // Whitespace means current name string has ended
                    }

                    else{
                        name[len++] = cur;          // Add everything else to name string
                    }
                }
            }

            if(quote_flag) name_invalid = 1;        // If there was an open quote but no close quotes, the command is invalid

            if(!name_invalid){          // Valid commands are tokenised appropriately
                name[len] = '\0';
                tokens[token_count++] = (Token){TOKEN_NAME, name, i};
                i += len;
            }
            else{
                free(name);             // Malloced string is freed in case of invalid command
                break;
            }
        }

    }

    if(!name_invalid){
        tokens[token_count] = (Token){TOKEN_EOF, NULL, i};          // Ending array of tokens with TOKEN_EOF
        return tokens;          // Return array of tokens
    }
    
    free(tokens);       // Freeing malloced array in case of invalid command and returning NULL
    return NULL;
}

ShellCmdNode *new_shell_cmd_node(){
    ShellCmdNode *node = (ShellCmdNode *)malloc(sizeof(ShellCmdNode));

    node->background = 0;
    node->count = 0;
    node->operators = (char *)malloc(sizeof(char) * MAX_TOKENS);
    node->cmd_groups = (CmdGroupNode **)malloc(sizeof(CmdGroupNode *) * MAX_TOKENS);

    return node;
}

int next_token(Token *tokens){
    Token *next = tokens + 1;
    return next->type == TOKEN_NAME;
}

ShellCmdNode *parse_shell_cmd(Token **tokens){
    ShellCmdNode *node = new_shell_cmd_node();

    node->cmd_groups[node->count++] = parse_cmd_groups(tokens);     // Parsing the first cmd_group

    if(node->cmd_groups[node->count - 1] == NULL){
        free(node->cmd_groups);
        free(node->operators);
        free(node);
        return NULL;
    };      // Returning NULL in case of invalid cmd_group

    while(((*tokens)->type == TOKEN_AND && next_token(*tokens)) || (*tokens)->type == TOKEN_SEMICOLON){      // Reading cmd_groups as long as there are ; and & separating them
        char op = ((*tokens)->type == TOKEN_AND) ? '&' : ';';
        (*tokens)++;        // Move to next token after the operator
        node->operators[node->count - 1] = op;
        node->cmd_groups[node->count++] = parse_cmd_groups(tokens);

        if(node->cmd_groups[node->count - 1] == NULL){          // Return NULL if the previously read cmd_group was invalid
            free(node->cmd_groups);
            free(node->operators);
            free(node);
            return NULL;
        }
    }

    if((*tokens)->type == TOKEN_AND){       // Reading if the command has the trailing & or not
        node->background = 1;
        (*tokens)++;
    }

    if((*tokens)->type != TOKEN_EOF){
        free(node->cmd_groups);
        free(node->operators);
        free(node);
        return NULL;
    }

    return node;
}

CmdGroupNode *new_cmd_group_node(){
    CmdGroupNode *node = (CmdGroupNode *)malloc(sizeof(CmdGroupNode));

    node->atomics = (AtomicNode **)malloc(sizeof(AtomicNode *) * MAX_TOKENS);
    node->count = 0;

    return node;
}

CmdGroupNode *parse_cmd_groups(Token **tokens){
    CmdGroupNode *node = new_cmd_group_node();

    node->atomics[node->count++] = parse_atomics(tokens);       // Parsing the first atomic

    if(node->atomics[node->count - 1] == NULL){
        free(node->atomics);
        free(node);
        return NULL;
    }      // Returning NULL in case of invalid atomic

    while((*tokens)->type == TOKEN_PIPE){
        (*tokens)++;        // Move to the next token after pipe
        node->atomics[node->count++] = parse_atomics(tokens);

        if(node->atomics[node->count - 1] == NULL){         // Freeing properly before returning NULL in case of invalid atomic
            for(int i = 0; i < node->count; i++){
                free(node->atomics[i]);
            }
            free(node->atomics);
            free(node);
            return NULL;
        }
    }

    return node;
}

AtomicNode *new_atomic_node(){
    AtomicNode *node = (AtomicNode *)malloc(sizeof(AtomicNode));    
    
    node->argv = (char **)malloc(sizeof(char *) * MAX_TOKENS);
    node->argc = 0;
    node->files = (char **)malloc(sizeof(char *) * MAX_TOKENS);
    node->op = (char **)malloc(sizeof(char *) * MAX_TOKENS);
    node->count = 0;

    return node;
}

AtomicNode *parse_atomics(Token **tokens){
    AtomicNode *node = new_atomic_node();

    if((*tokens)->type != TOKEN_NAME){      // Must start with name
        free(node->argv);
        free(node->files);
        free(node->op);
        free(node);

        return NULL;
    }

    node->argv[node->argc++] = parse_name(tokens);
    if(!node->argv[node->argc - 1]){
        for(int i = 0; i < node->argc; i++){
            free(node->argv[i]);
        }
        free(node->argv);
        free(node->files);
        free(node->op);
        free(node);

        return NULL;            // Return NULL if current argument is invalid
    }

    while(1){
        if((*tokens)->type == TOKEN_NAME){
            node->argv[node->argc++] = parse_name(tokens);

            if(!node->argv[node->argc - 1]){
                for(int i = 0; i < node->argc; i++){
                    free(node->argv[i]);
                }
                free(node->argv);
                
                for(int i = 0; i < node->count; i++){
                    free(node->files[i]);
                }
                free(node->files);
                free(node->op);
                
                free(node);

                return NULL;            // Return NULL if current argument is invalid
            }
        }

        else if((*tokens)->type == TOKEN_INPUT || (*tokens)->type == TOKEN_OUTPUT || (*tokens)->type == TOKEN_APPEND){     // If next token is an input token
            char *operator = (*tokens)->type == TOKEN_INPUT ? "<" : ((*tokens)->type == TOKEN_OUTPUT ? ">" : ">>");
            (*tokens)++;        // Consume operator
            char *file_name = parse_name(tokens);      // Read file name

            if(!file_name){
                for(int i = 0; i < node->argc; i++){
                    free(node->argv[i]);
                }
                free(node->argv);
                
                for(int i = 0; i < node->count; i++){
                    free(node->files[i]);
                }
                free(node->files);
                free(node->op);
                
                free(node);

                return NULL;            // Return NULL if current argument is invalid
            }
            
            node->files[node->count] = file_name;
            node->op[node->count] = operator;
            node->count++;
        }

        else{
            break;
        }
    }

    node->argv[node->argc] = NULL;      // NULL terminate arguments array

    return node;
}

char *parse_name(Token **tokens){
    if((*tokens)->type != TOKEN_NAME) return NULL;

    char *name = (char *)malloc(strlen((*tokens)->token) + 1);
    strcpy(name, (*tokens)->token);
    (*tokens)++;

    return name;
}