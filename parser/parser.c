#include "parser.h"

static long totalAdvCall = 0;

Token peek(Parser *p) {
    return p->tokens->tokens[p->pos];
}

Token advance(Parser *p) {
    totalAdvCall++;
    if(totalAdvCall > 1000000){
        printf("ПОМИЛКА: забагато викликів advance(), pos=%d token='%s'\n",
        p->pos, peek(p).text);
        exit(1);
    }
    return p->tokens->tokens[p->pos++];
}

bool check(Parser *p, TokenType type) {
    return peek(p).type == type;
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

// factor := NUMBER | TEXT | '(' expr ')'
Node *parseFactor(Parser *p) {
    Token t = peek(p);

    if (t.type == TOK_NUMBER) {
        advance(p);
        Node *n = newNode(NODE_NUMBER);
        n->number = atoi(t.text);
        return n;
    }

    if (t.type == TOK_STRING) {
        advance(p);
        Node *n = newNode(NODE_STRING);
        strcpy(n->text, t.text);
        return n;
    }

    if (t.type == TOK_FLOAT) {
        advance(p);
        Node *n = newNode(NODE_FLOAT);
        n->fvalue = atof(t.text);  
        return n;
    }
    if (t.type == TOK_TRUE) {
        advance(p);
        Node *n = newNode(NODE_BOOL);
        n->number = 1;
        return n;
    }

    if (t.type == TOK_FALSE) {
        advance(p);
        Node *n = newNode(NODE_BOOL);
        n->number = 0;
        return n;
    }

    if (t.type == TOK_TEXT) {
        advance(p);

        // func: TEXT '(' args? ')'
        if (check(p, TOK_LPAREN)) {
            advance(p); // '('
            Node *n = newNode(NODE_CALL);
            strcpy(n->text, t.text);
            n->args = NULL;
            n->argCount = 0;

            if (!check(p, TOK_RPAREN)) {
                Node *arg = parseExpr(p);
                n->args = realloc(n->args, sizeof(Node*) * (n->argCount + 1));
                n->args[n->argCount++] = arg;

                while (check(p, TOK_COMMA)) {
                    advance(p);
                    arg = parseExpr(p);
                    n->args = realloc(n->args, sizeof(Node*) * (n->argCount + 1));
                    n->args[n->argCount++] = arg;
                }
            }

            expect(p, TOK_RPAREN, ")");
            return n;
        }

        // звичайна змінна
        Node *n = newNode(NODE_TEXT);
        strcpy(n->text, t.text);
        return n;
    }

    if (t.type == TOK_LPAREN) {
        advance(p);
        Node *n = parseExpr(p);
        expect(p, TOK_RPAREN, ")");
        return n;
    }

    printf("Unknown symbol '%c' in line %d\n", t.text, t.line);
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

// comparison := arith (('<' | '>' | ...) arith)?
Node *parseComparison(Parser *p) {
    Node *left = parseArith(p);  

    if (check(p, TOK_LT) || check(p, TOK_GT) || check(p, TOK_LE) ||
        check(p, TOK_GE) || check(p, TOK_EQ) || check(p, TOK_NE)) {
        Token op = advance(p);
        Node *right = parseArith(p);
        Node *n = newNode(NODE_COMPARE);
        n->op = op.type;
        n->left = left;
        n->right = right;
        return n;
    }

    return left;
}

Node *parseExpr(Parser *p) {
    return parseComparison(p);
}

// expr := term (('+' | '-') term)*
Node *parseArith(Parser *p) {
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

// program := (funcDecl | statement)*
Node *parseProgram(Parser *p) {
    Node *n = newNode(NODE_BLOCK);
    n->statements = NULL;
    n->stmtCount = 0;
    int safety = 0;

    while (!check(p, TOK_EOF)) {
        Node *stmt;
        if (++safety > 10000) {       
            printf("ПОМИЛКА: нескінченний цикл парсингу на pos=%d token='%s'\n",
                   p->pos, peek(p).text);
            exit(1);
        }
        if (check(p, TOK_KW_FUNC)) {
            stmt = parseFuncDecl(p);
        } else {
            stmt = parseStatement(p);
        }
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
    printf(">>> pos=%d token='%s' type=%d (TOK_TEXT=%d)\n",
     p->pos, peek(p).text, peek(p).type, TOK_TEXT);

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

    // ASING BLOCK
    if (check(p, TOK_TEXT)) {
         printf(">>> TEXT block: pos=%d next_type=%d TOK_ASSIGN=%d count=%d\n",
           p->pos,
           p->tokens->tokens[p->pos + 1].type,
           TOK_ASSIGN,
           p->tokens->count); 
        // Look one token ahead, without changing the current parser position
        // If the next token exists and is an '=' sign
        if (p->pos + 1 < p->tokens->count && p->tokens->tokens[p->pos + 1].type == TOK_ASSIGN) {
             printf(">>> calling parseAssignOrExpr\n");
            return parseAssignOrExpr(p); 
        } else {
            // this call func (add(x, y);)
            printf(">>> calling parseExpr\n");
            Node *e = parseExpr(p);
            expect(p, TOK_SEMI, "expected ';' after expression");
            
            Node *n = newNode(NODE_EXPR_STMT);
            n->expr = e;
            return n;
        }
    }

    printf("Parse error: unexpected token '%s' on line %d\n",
           peek(p).text, peek(p).line);
    exit(1);
}

// name = INT, STRING, FLOAT, BOOL, ARRAY
Node *parseAssignOrExpr(Parser *p) {
    Token ident = advance(p); // IDENT

    expect(p, TOK_ASSIGN, "=");

    Node *n = newNode(NODE_ASSIGN);
    strcpy(n->text, ident.text);
    n->left = parseExpr(p);

    expect(p, TOK_SEMI, ";");
    return n;
}

// if(expr GATE NOT WORK){ ... } else{ ... }
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

// while(expr){ ... }
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

// func name(params); params only int and max 8
Node *parseFuncDecl(Parser *p) {
    expect(p, TOK_KW_FUNC, "func");
    Token name = expect(p, TOK_TEXT, "name func");

    Node *n = newNode(NODE_FUNC_DECL);
    strcpy(n->text, name.text);
    n->paramCount = 0;

    expect(p, TOK_LPAREN, "(");
    if (!check(p, TOK_RPAREN)) {
        Token param = expect(p, TOK_TEXT, "params");
        strcpy(n->params[n->paramCount++], param.text);

        while (check(p, TOK_COMMA)) {
            advance(p);
            param = expect(p, TOK_TEXT, "params");
            strcpy(n->params[n->paramCount++], param.text);
        }
    }
    expect(p, TOK_RPAREN, ")");

    n->thenBranch = parseBlock(p); // block func
    return n;
}

// return expr; expr only int, i working on this
Node *parseReturn(Parser *p) {
    expect(p, TOK_KW_RETURN, "return");
    Node *n = newNode(NODE_RETURN);
    n->expr = parseExpr(p);
    expect(p, TOK_SEMI, ";");
    return n;
}

// print expr; expr = INT, STRING, FLOAT
Node *parsePrint(Parser *p){
    advance(p); 
    Node *n = newNode(NODE_PRINT);
    if (check(p, TOK_STRING)) {
        Token str = advance(p);
        Node *strNode = newNode(NODE_STRING);
        strcpy(strNode->text, str.text);
        n->expr = strNode;
    } else {
        n->expr = parseExpr(p);
    }
    expect(p, TOK_SEMI, ";");
    return n;
}

// `this you can comment
Node *parseIgnor(Parser *p) {
    int commentLine = peek(p).line;
    advance(p);  // споживаємо `

    Node *n = newNode(NODE_IGNOR);
    n->text[0] = '\0';

    // споживаємо всі токени на тому ж рядку
    while (!check(p, TOK_EOF) && peek(p).line == commentLine) {
        // збираємо текст коментаря якщо хочеш зберегти
        if (strlen(n->text) + strlen(peek(p).text) + 2 < sizeof(n->text)) {
            strcat(n->text, peek(p).text);
            strcat(n->text, " ");
        }
        advance(p);
    }

    return n;
}