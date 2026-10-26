#include "parser.h"

Token peek(Parser *p) {
    return p->tokens->tokens[p->pos];
}

Token advance(Parser *p) {
    return p->tokens->tokens[p->pos++];
}

bool check(Parser *p, TokenType type) {
    Token t = peek(p);

    return t.type == type;
}

Token expect(Parser *p, TokenType type, const char *msg) {
    if (!check(p, type)) {
        printf("Parsing error: expected %s, line %d\n", msg, peek(p).line);
        exit(1);
    }
    return advance(p);
}

Node *newNode(NodeType type) {
    Node *n = calloc(1, sizeof(Node));
    n->type = type;
    return n;
}

// program := (funcDecl | statement)*
Node *parseProgram(Parser *p) {
    Node *n = newNode(NODE_BLOCK);
    n->statements = NULL;
    n->stmtCount = 0;

    while (!check(p, TOK_EOF)) {
        Node *stmt;
        if (check(p, TOK_KW_IMPORT)) stmt = parseImport(p);
        else if (check(p, TOK_KW_FUNC)) stmt = parseFuncDecl(p);
        else stmt = parseStatement(p);
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

    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {  // protected from absence }
        Node *stmt = parseStatement(p);
        n->statements = realloc(n->statements, sizeof(Node*) * (n->stmtCount + 1));
        n->statements[n->stmtCount++] = stmt;
    }

    expect(p, TOK_RBRACE, "}");
    return n;
}

// statement := assign | print | if | while | block
Node *parseStatement(Parser *p) {

    // IF BLOCK
    if (check(p, TOK_KW_IF))     return parseIf(p);

    // WHILE BLOCK
    if (check(p, TOK_KW_WHILE))  return parseWhile(p);

    // RETURN
    if (check(p, TOK_KW_RETURN)) return parseReturn(p);

    // PRINT BLOCK
    if (check(p, TOK_KW_PRINT))  return parsePrint(p);

    //COMMENT BLOCK
    if(check(p, TOK_IGNOR))      return parseIgnor(p);

    // BLOCK '{ ... }'
    if (check(p, TOK_LBRACE))    return parseBlock(p);

    //ARRAY BLOCK
    if (check(p, TOK_LBRACKET))  return parseArray(p);

    //FOR BLOCK
    if(check(p, TOK_KW_FOR))     return parseFor(p);

    //IMPORT BLOCK
    if(check(p, TOK_KW_IMPORT))  return parseImport(p);

    // C code block: @{ ... }
    if (check(p, TOK_RAW_C)) {
        Token t = advance(p);
        Node *n = newNode(NODE_RAW_C);
        n->text = _strdup(t.text);
        return n;
    }

    // DEREF ASSIGN: *p = value;
    if (check(p, TOK_STAR)) {
        int save = p->pos;
        Node *e = parseExpr(p);

        if (check(p, TOK_ASSIGN) && e->type == NODE_DEREF) {
            advance(p); // '='
            Node *val = parseExpr(p);
            expect(p, TOK_SEMI, ";");

            Node *n = newNode(NODE_DEREF_ASSIGN);
            n->left  = e;
            n->right = val;
            return n;
        }

        p->pos = save;
        e = parseExpr(p);
        expect(p, TOK_SEMI, ";");
        Node *n = newNode(NODE_EXPR_STMT);
        n->expr = e;
        return n;
    }

    // ASING BLOCK
    //rewrite for assing array and var
    if (check(p, TOK_TEXT)) {
        int save = p->pos;
        Token name = advance(p); //name array or var

        if (check(p, TOK_LBRACKET)) {
            // arr[i] = value or arr[i]
            advance(p);              // '['
            Node *idx = parseExpr(p);
            expect(p, TOK_RBRACKET, "]");

            if (check(p, TOK_ASSIGN)) {
                // arr[i] = value
                advance(p); // '='
                Node *val = parseExpr(p);
                expect(p, TOK_SEMI, ";");

                Node *n = newNode(NODE_INDEX_ASSIGN);
                n->text  = _strdup(name.text);
                n->line  = name.line;
                n->left  = idx;
                n->right = val;
                return n;
            }

            //just arr[i]; as expression
            p->pos = save;
            Node *e = parseExpr(p);
            expect(p, TOK_SEMI, ";");
            Node *n = newNode(NODE_EXPR_STMT);
            n->expr = e;
            return n;
        }

        if (check(p, TOK_ASSIGN)) {
            //ordinary assignment x = ...
            p->pos = save;
            return parseAssignOrExpr(p);
        }

        //func call or expression
        p->pos = save;
        Node *e = parseExpr(p);
        expect(p, TOK_SEMI, ";");
        Node *n = newNode(NODE_EXPR_STMT);
        n->expr = e;
        return n;
    }

    printf("Parse error: unexpected token '%s' on line %d\n",
           peek(p).text, peek(p).line);
    exit(1);
}

Node *parseImport(Parser *p) {
    expect(p, TOK_KW_IMPORT, "import");
    Token path = expect(p, TOK_STRING, "import path");
    expect(p, TOK_SEMI, ";");

    Node *n = newNode(NODE_IMPORT);
    n->text = _strdup(path.text);

    // Parse Python module immediately
    size_t len = strlen(path.text);
    if (len > 3 && strcmp(path.text + len - 3, ".py") == 0) {
        PyModule mod = parsePyFile(path.text);

        strncpy(pyModuleNames[pyModuleCount], mod.name,
                sizeof(pyModuleNames[pyModuleCount]) - 1);
        pyModuleNames[pyModuleCount][sizeof(pyModuleNames[pyModuleCount]) - 1] = '\0';

        pyModules[pyModuleCount] = mod;
        pyModuleCount++;
    }

    return n;
}

void freeNode(Node *n)
{
    if (!n)
        return;

    freeNode(n->left);
    freeNode(n->right);
    freeNode(n->cond);
    freeNode(n->thenBranch);
    freeNode(n->elseBranch);
    freeNode(n->expr);

    for (int i = 0; i < n->stmtCount; i++)
        freeNode(n->statements[i]);

    for (int i = 0; i < n->argCount; i++)
        freeNode(n->args[i]);

    free(n->statements);
    free(n->args);

    free(n->module);
    free(n->text);

    free(n);
}

void freePyModules()
{
    for (int m = 0; m < pyModuleCount; m++) {
        for (int i = 0; i < pyModules[m].count; i++) {
            free(pyModules[m].funcs[i].name);
        }

        free(pyModules[m].name);
        pyModules[m].name = NULL;
    }

    pyModuleCount = 0;
}
