#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum TokenType{
    TOK_NUMBER,
    TOK_TEXT,
    TOK_STRING,
    TOK_FLOAT,
    TOK_ASSIGN,     // =
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH,
//  ↓<      ↓>       ↓<=     ↓>=     ↓=      ↓!      ↓++      ↓--    
    TOK_LT, TOK_GT, TOK_LE, TOK_GE, TOK_EQ, TOK_NE, TOK_INC, TOK_DEC,  
//  |(                   )| |{                   }| |[                       ]| 
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE, TOK_LBRACKET, TOK_RBRACKET,
    TOK_SEMI,                 // ;
    TOK_COMMA,
    TOK_KW_IF, TOK_KW_ELSE, TOK_KW_WHILE, TOK_KW_FOR, TOK_KW_PRINT,
    TOK_KW_FUNC, TOK_KW_RETURN,
    TOK_EOF,
    TOK_RAW_C,              // raw C code
    TOK_IGNOR,              //`comments
    TOK_TRUE, TOK_FALSE,
    TOK_UNKNOWN,
} TokenType;

typedef struct Token{
    TokenType type;
    char text[4096];
    int line;       // для повідомлень про помилки
} Token;

typedef struct TokenList {
    Token *tokens;
    int count;
    int capacity;
} TokenList;

TokenList tokenize(const char *src, size_t len);