#ifndef PARSER_H
#define PARSER_H

#define MAX_TOKENS 4096
#define MAX_NAME_LEN 4096

// This is for classifying each token of a command into name or the other functional symbols
typedef enum{
    TOKEN_NAME,
    TOKEN_PIPE,      // |
    TOKEN_AND,       // &
    TOKEN_SEMICOLON, // ;
    TOKEN_INPUT,     // <
    TOKEN_OUTPUT,    // >
    TOKEN_APPEND,    // >>
    TOKEN_EOF
} token_type;

// This is for representing a token in the input
typedef struct Token{
    token_type type;    // Type of the token
    char *token;        // Name string, NULL for other types
    int position;       // To identify where an error occurred 
} Token;

typedef struct AtomicNode{
    char **argv;          // Arguments array
    int argc;             // Length of arguments array
    char **files;         // For input and output redirection, NULL if none
    char **op;            // Array of operators '>' or '<' or '>>'
    int count;            // Number of operators 
} AtomicNode;

typedef struct CmdGroupNode{
    AtomicNode **atomics;   // Array of atomic commands
    int count;              // No. of atomic commands
} CmdGroupNode;

typedef struct ShellCmdNode{
    CmdGroupNode **cmd_groups;  // Array of cmd_groups (separated by ; or &)
    char *operators;            // Array of operators- ; and &
    int count;                  // No. of cmd_groups
    int background;             // 1 if there is trailing &, 0 otherwise
} ShellCmdNode;

int is_whitespace(char c);

Token *tokenise(char *input);

ShellCmdNode *new_shell_cmd_node();

int next_token(Token *tokens);

ShellCmdNode *parse_shell_cmd(Token **tokens);

CmdGroupNode *new_cmd_group_node();

CmdGroupNode *parse_cmd_groups(Token **tokens);

AtomicNode *new_atomic_node();

AtomicNode *parse_atomics(Token **token);

char *parse_name(Token **tokens);

#endif