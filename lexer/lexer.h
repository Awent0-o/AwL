#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum TokenType{
//  ↓1 2 3 4 5  ↓text     ↓"string"   ↓f""         ↓1.2       ↓=
    TOK_NUMBER, TOK_TEXT, TOK_STRING, TOK_FSTRING, TOK_FLOAT, TOK_ASSIGN,     
//  ↓+        ↓-         ↓*        ↓/          ↓++      ↓-- 
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_INC, TOK_DEC, 
//  ↓<      ↓>       ↓<=     ↓>=     ↓=      ↓!         
    TOK_LT, TOK_GT, TOK_LE, TOK_GE, TOK_EQ, TOK_NE,
//  |(                   )| |{                   }| |[                       ]| 
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE, TOK_LBRACKET, TOK_RBRACKET,
//  ↓;        ↓,         ↓.
    TOK_SEMI, TOK_COMMA, TOK_DOT,
//defulat KW
            TOK_KW_IF, TOK_KW_ELSE, TOK_KW_WHILE, TOK_KW_FOR, TOK_KW_PRINT, TOK_KW_IMPORT, 
//FUNC RETURN KW
            TOK_KW_INT, TOK_KW_FLOAT, TOK_KW_STRING, TOK_KW_BOOL, TOK_KW_VOID,
//FUNC DEFUALT KW
            TOK_KW_FUNC, TOK_KW_RETURN, TOK_ARROW,
    TOK_EOF,                // empty 
    TOK_RAW_C,              // raw C code
    TOK_IGNOR,              //`comments
    TOK_TRUE, TOK_FALSE,    // true, false return 1 or 0
    TOK_UNKNOWN,            
} TokenType;

typedef struct Token{

    TokenType type;

    char text[4096];

    //for semantic
    int line;

    int colume;

    char* file;

} Token;

typedef struct TokenList {
    Token *tokens;
    int count;
    int capacity;
} TokenList;

TokenList tokenize(const char *src, size_t len);

#endif