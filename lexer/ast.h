#ifndef AST_H
#define AST_H

#include "lexer.h"

typedef enum {
    TYPE_UNKNOWN = -1,
    TYPE_INT = 0,
    TYPE_STRING = 1,
    TYPE_FLOAT = 2,
    TYPE_BOOL = 3,
    TYPE_ARRAY = 4,
    TYPE_AUTO = 5,
    TYPE_CALL = 6,
    TYPE_VOID = 7
} DataType;

typedef struct {
    char *name; //<- not limit name func
    DataType type;
} Param;

typedef enum NodeType{
//          ↓1 2 3 4 5   ↓"string"    ↓f"{}"        ↓print expr expr ↓text      ↓1+2=3      ↓1>2           ↓a = 1       ↓a = 1<-create
            NODE_NUMBER, NODE_STRING, NODE_FSTRING, NODE_PRINT_LIST, NODE_TEXT, NODE_BINOP, NODE_COMPARE, NODE_ASSIGN, NODE_VAR_DEC,
//DEFUALT KW
            NODE_PRINT, NODE_IF, NODE_WHILE, NODE_FOR, NODE_BLOCK, NODE_IMPORT,
//DEFUALT FUNC KW
            NODE_FUNC_DECL, NODE_CALL, NODE_RETURN, NODE_PY_CALL, //<- only for semantic
//TYPE FOR RETURN
            NODE_FLOAT, NODE_BOOL, NODE_ARRAY, NODE_INDEX, NODE_INDEX_ASSIGN,

    NODE_EXPR_STMT,
    NODE_IGNOR,     //`commentc
    NODE_RAW_C      // <-- for raw C code block
} NodeType;

typedef struct Node {

    Token token;
    NodeType type;
    int number;
    char *text;
    char *module;

    TokenType op;
    bool isDeclaration;

    struct Node *left, *right;

    struct Node *cond;

    struct Node *thenBranch, *elseBranch;

    struct Node *expr;        // <- print/assign

    struct Node **statements;
    int stmtCount;

    Param *params;         // <- not limit params
    int paramCount;
    int paramCapacity;
    DataType returnType;

    struct Node **args;       // <-- argument performs
    int argCount;

    float fvalue; // for float values

} Node;

#endif