#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

typedef enum TokenType{
    TOK_NUMBER,
    TOK_IDENT,
    TOK_ASSIGN,     // =
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH,
    TOK_LPAREN, TOK_RPAREN,
    TOK_LBRACE, TOK_RBRACE,   // { }
    TOK_SEMI,                 // ;
    TOK_KW_IF, TOK_KW_ELSE, TOK_KW_WHILE, TOK_KW_PRINT,
    TOK_EOF,
    TOK_UNKNOWN
} TokenType;

typedef struct Token{
    TokenType type;
    char text[64];
    int line;       // для повідомлень про помилки
} Token;

typedef struct TokenList {
    Token *tokens;
    int count;
    int capacity;
} TokenList;


typedef enum NodeType{
    NODE_NUMBER, NODE_IDENT, NODE_BINOP,
    NODE_ASSIGN, NODE_PRINT, NODE_IF, NODE_WHILE, NODE_BLOCK
} NodeType;

typedef struct Node {

    NodeType type;
    int number;
    char ident[64];

    TokenType op;

    struct Node *left, *right;

    struct Node *cond;

    struct Node *thenBranch, *elseBranch;

    struct Node *expr;        // <- окреме поле для print/assign-значення

    struct Node **statements;
    int stmtCount;
} Node;


TokenList tokenize(const char *src, size_t len);