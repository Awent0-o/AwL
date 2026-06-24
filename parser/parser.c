#include "parser.h"

Token peek(Parser *p) {
    return p->tokens->tokens[p->pos];
}

Token advance(Parser *p) {
    return p->tokens->tokens[p->pos++];
}

bool check(Parser *p, TokenType type) {
    return peek(p).type == type;
}

Token expect(Parser *p, TokenType type, const char *msg) {
    if (!check(p, type)) {
        printf("Помилка парсингу: очікувалось %s, рядок %d\n", msg, peek(p).line);
        exit(1);
    }
    return advance(p);
}

Node *newNode(NodeType type) {
    Node *n = calloc(1, sizeof(Node));
    n->type = type;
    return n;
}

// factor := NUMBER | IDENT | '(' expr ')'
Node *parseFactor(Parser *p) {
    Token t = peek(p);

    if (t.type == TOK_NUMBER) {
        advance(p);
        Node *n = newNode(NODE_NUMBER);
        n->number = atoi(t.text);
        return n;
    }
    if (t.type == TOK_IDENT) {
        advance(p);
        Node *n = newNode(NODE_IDENT);
        strcpy(n->ident, t.text);
        return n;
    }
    if (t.type == TOK_LPAREN) {
        advance(p);
        Node *n = parseExpr(p);
        expect(p, TOK_RPAREN, ")");
        return n;
    }

    printf("Неочікуваний токен '%s' на рядку %d\n", t.text, t.line);
    exit(1);
}

// term := factor (('*' | '/') factor)*
Node *parseTerm(Parser *p) {
    Node *left = parseFactor(p);
    while (check(p, TOK_STAR) || check(p, TOK_SLASH)) {
        Token op = advance(p);
        Node *right = parseFactor(p);
        Node *n = newNode(NODE_BINOP);
        n->op = op.type;
        n->left = left;
        n->right = right;
        left = n;
    }
    return left;
}

// expr := term (('+' | '-') term)*
Node *parseExpr(Parser *p) {
    Node *left = parseTerm(p);
    while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
        Token op = advance(p);
        Node *right = parseTerm(p);
        Node *n = newNode(NODE_BINOP);
        n->op = op.type;
        n->left = left;
        n->right = right;
        left = n;
    }
    return left;
}
// program := statement* EOF
Node *parseProgram(Parser *p) {
    Node *n = newNode(NODE_BLOCK);
    n->statements = NULL;
    n->stmtCount = 0;

    while (!check(p, TOK_EOF)) {
        Node *stmt = parseStatement(p);
        n->statements = realloc(n->statements, sizeof(Node*) * (n->stmtCount + 1));
        n->statements[n->stmtCount++] = stmt;
    }

    return n;
}

// block := '{' statement* '}'
Node *parseBlock(Parser *p) {
    expect(p, TOK_LBRACE, "{");

    Node *n = newNode(NODE_BLOCK);
    n->statements = NULL;
    n->stmtCount = 0;

    while (!check(p, TOK_RBRACE)) {
        Node *stmt = parseStatement(p);
        n->statements = realloc(n->statements, sizeof(Node*) * (n->stmtCount + 1));
        n->statements[n->stmtCount++] = stmt;
    }

    expect(p, TOK_RBRACE, "}");
    return n;
}

// statement := assign | print | if | while | block
Node *parseStatement(Parser *p) {
    if (check(p, TOK_KW_IF)) {
        return parseIf(p);
    }
    if (check(p, TOK_KW_WHILE)) {
        return parseWhile(p);
    }
    if (check(p, TOK_KW_PRINT)) {
        advance(p); // 'print'
        Node *n = newNode(NODE_PRINT);
        n->left = parseExpr(p);
        expect(p, TOK_SEMI, ";");
        return n;
    }
    if (check(p, TOK_LBRACE)) {
        return parseBlock(p);
    }
    if (check(p, TOK_IDENT)) {
        return parseAssignOrExpr(p);
    }

    printf("Помилка парсингу: неочікуваний токен '%s' на рядку %d\n",
           peek(p).text, peek(p).line);
    exit(1);
}

// assign := IDENT '=' expr ';'
Node *parseAssignOrExpr(Parser *p) {
    Token ident = advance(p); // IDENT

    expect(p, TOK_ASSIGN, "=");

    Node *n = newNode(NODE_ASSIGN);
    strcpy(n->ident, ident.text);
    n->left = parseExpr(p);

    expect(p, TOK_SEMI, ";");
    return n;
}

// if := 'if' '(' expr ')' block ('else' block)?
Node *parseIf(Parser *p) {
    expect(p, TOK_KW_IF, "if");
    expect(p, TOK_LPAREN, "(");
    Node *cond = parseExpr(p);
    expect(p, TOK_RPAREN, ")");

    Node *n = newNode(NODE_IF);
    n->cond = cond;
    n->thenBranch = parseBlock(p);

    if (check(p, TOK_KW_ELSE)) {
        advance(p);
        n->elseBranch = parseBlock(p);
    } else {
        n->elseBranch = NULL;
    }

    return n;
}

// while := 'while' '(' expr ')' block
Node *parseWhile(Parser *p) {
    expect(p, TOK_KW_WHILE, "while");
    expect(p, TOK_LPAREN, "(");
    Node *cond = parseExpr(p);
    expect(p, TOK_RPAREN, ")");

    Node *n = newNode(NODE_WHILE);
    n->cond = cond;
    n->thenBranch = parseBlock(p);

    return n;
}