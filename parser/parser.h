#include "../lexer/lexer.h"
#include "../lexer/ast.h"
#include <stdbool.h>

typedef struct Parser{
    TokenList *tokens;
    int pos;
} Parser;

Token peek(Parser *p);

Token advance(Parser *p);

bool check(Parser *p, TokenType type);

Token expect(Parser *p, TokenType type, const char* msg);

Node *newNode(NodeType type);

Node *parseFactor(Parser *p);

Node *parseTerm(Parser *p);

Node *parseExpr(Parser *p);

Node *parseProgram(Parser *p);

Node *parseStatement(Parser *p);

Node *parseBlock(Parser *p);

Node *parseIf(Parser *p);

Node *parseWhile(Parser *p);

Node *parseAssignOrExpr(Parser *p);

Node *parseArith(Parser *p);

Node *parseComparison(Parser *p);

Node *parseFuncDecl(Parser *p);

Node *parseReturn(Parser *p);

Node *parsePrint(Parser *p);

Node *parseIgnor(Parser *p);

Node *parseArray(Parser *p);