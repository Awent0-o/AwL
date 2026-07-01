#include "lexer.h"

typedef enum NodeType{
    NODE_NUMBER, NODE_STRING, NODE_TEXT, NODE_BINOP, NODE_COMPARE,
    NODE_ASSIGN, NODE_PRINT, NODE_IF, NODE_WHILE, NODE_BLOCK,
    NODE_FUNC_DECL, NODE_CALL, NODE_RETURN,
    NODE_FLOAT, NODE_BOOL, //NODE_ARRAY,
    NODE_EXPR_STMT, 
    NODE_IGNOR, // for comments
    NODE_INDEX // index array
} NodeType;

typedef struct Node {

    NodeType type;
    int number;
    char text[256]; 

    TokenType op;

    struct Node *left, *right;

    struct Node *cond;

    struct Node *thenBranch, *elseBranch;

    struct Node *expr;        // <- print/assign

    struct Node **statements;
    int stmtCount;

    char params[8][64];       // <-- params func (max 8)
    int paramCount;

    struct Node **args;       // <-- argument performs
    int argCount;

    float fvalue; // for float values

} Node;