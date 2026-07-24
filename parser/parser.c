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

    if (!n) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    n->type = type;
    n->text = NULL;
    return n;
}

// factor := NUMBER | TEXT | '(' expr ')'
Node *parseFactor(Parser *p) {
    Token t = peek(p);

    if (t.type == TOK_MINUS) {
        advance(p);
        Token num = peek(p);

        if (num.type == TOK_NUMBER) {
            advance(p);
            Node *n = newNode(NODE_NUMBER);
            n->number = -atoi(num.text);
            return n;
        }
        if (num.type == TOK_FLOAT) {
            advance(p);
            Node *n = newNode(NODE_FLOAT);
            n->fvalue = -(float)atof(num.text);
            return n;
        }

        // -expr: -(a + b)
        // gen (0 - expr)
        Node *zero = newNode(NODE_NUMBER);
        zero->number = 0;
        Node *right = parseFactor(p);
        Node *n = newNode(NODE_BINOP);
        n->op    = TOK_MINUS;
        n->left  = zero;
        n->right = right;
        return n;
    }

    if (t.type == TOK_NUMBER) {
        advance(p);
        Node *n = newNode(NODE_NUMBER);
        n->number = atoi(t.text);
        return n;
    }

    if (t.type == TOK_STRING) {
        advance(p);
        Node *n = newNode(NODE_STRING);
        n->text = _strdup(t.text);
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

    if (t.type == TOK_LBRACKET){
        return parseArray(p);
    }

    if (t.type == TOK_TEXT) {
        advance(p);

        // array index: Name = [expr, expr2];
        if (check(p, TOK_LBRACKET)) {
            advance(p);  // '['
            Node *n = newNode(NODE_INDEX);
            n->text = _strdup(t.text); // name
            n->left = parseExpr(p);    // index
            expect(p, TOK_RBRACKET, "]");
            return n;
        }

        //lib.func(args) — call py func
        if (check(p, TOK_DOT)) {
            advance(p);  // '.'
            Token func = expect(p, TOK_TEXT, "function name");
            expect(p, TOK_LPAREN, "(");

            Node *n = newNode(NODE_PY_CALL);

            //name lib
            n->module = _strdup(t.text);
            
            // name func
            n->text = _strdup(func.text);

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

        // func: Name '(' args? ')'
        if (check(p, TOK_LPAREN)) {
            advance(p); // '('
            Node *n = newNode(NODE_CALL);
            n->text = malloc(strlen(t.text) + 1);
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

        // defualt var
        Node *n = newNode(NODE_TEXT);
        n->text = malloc(strlen(t.text) + 1);
        strcpy(n->text, t.text);
        return n;
    }

    if (t.type == TOK_LPAREN) {
        advance(p);
        Node *n = parseExpr(p);
        expect(p, TOK_RPAREN, ")");
        return n;
    }

    printf("Unknown symbol '%s' in line %d\n", t.text, t.line);
    exit(1);
}

// * and / multiplication and division
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

// comparison <, >, <=, >=, ==, !=, true, false
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
    else if (check(p, TOK_TRUE) || check(p, TOK_FALSE)) {
        Token op = advance(p);
        Node *n = newNode(NODE_BOOL);
        n->number = (op.type == TOK_TRUE) ? 1 : 0;
        return n;
    }

    return left;
}

Node *parseExpr(Parser *p) {
    return parseComparison(p);
}

// +, -, ++, -- arrefmetic operations
Node *parseArith(Parser *p) {
    Node *left = parseTerm(p);
    while (check(p, TOK_PLUS) || check(p, TOK_MINUS) || check(p, TOK_INC) || check(p, TOK_DEC)){
        if (check(p, TOK_INC)) {
            advance(p);

            Node *n = newNode(NODE_BINOP);
            n->op = TOK_INC;
            n->left = left;
            left = n;
            continue;
        }

        if (check(p, TOK_DEC)) {
            advance(p);

            Node *n = newNode(NODE_BINOP);
            n->op = TOK_DEC;
            n->left = left;
            left = n;
            continue;
        }
        else{
            Token op = advance(p);
            Node *right = parseTerm(p);
            Node *n = newNode(NODE_BINOP);
            n->op = op.type;
            n->left = left;
            n->right = right;
            left = n;
        }
    }
    return left;
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

    // ASING BLOCK
    if (check(p, TOK_TEXT)) {
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

    Node *expr = parseExpr(p);
    expect(p, TOK_SEMI, ";");

    if(expr->type == NODE_ARRAY){
        Node *n = newNode(NODE_ARRAY);
        n->text = _strdup(ident.text);
        n->left = expr;
        return n;
    }

    Node *n = newNode(NODE_ASSIGN);
    n->text = _strdup(ident.text);
    n->left = expr;
    return n;
}

// if(expr GATE NOT WORK || &&){ ... } else{ ... }
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
    n->text = _strdup(name.text);
    n->paramCount = 0;
    n->returnType = TYPE_VOID;

    expect(p, TOK_LPAREN, "(");
    if (!check(p, TOK_RPAREN)) {
        DataType paramType = parseTypeAnnotation(p);
        Token param = expect(p, TOK_TEXT, "params");

        if (n->paramCount >= n->paramCapacity) {
            n->paramCapacity = (n->paramCapacity == 0) ? 4 : n->paramCapacity * 2;
        
            n->params = realloc(
                n->params,
                sizeof(Param) * n->paramCapacity
            );
        
            if (!n->params) {
                fprintf(stderr, "Out of memory\n");
                exit(1);
            }
        }
        
        n->params[n->paramCount].type = paramType;
        n->params[n->paramCount].name = _strdup(param.text);
        n->paramCount++;
        
        while (check(p, TOK_COMMA)) {
            advance(p);
            paramType = parseTypeAnnotation(p);
            param = expect(p, TOK_TEXT, "params");
            n->params[n->paramCount].type = paramType;
            n->params[n->paramCount].name = _strdup(param.text);
            n->paramCount++;
        }
    }
    expect(p, TOK_RPAREN, ")");

    if (check(p, TOK_ARROW)) {
        advance(p);  // '->'
        n->returnType = parseTypeAnnotation(p);
    }


    n->thenBranch = parseBlock(p); // block func
    return n;
}

// return expr; 
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

    Node *arg = NULL;
    Node *n = newNode(NODE_PRINT);
    while (!check(p, TOK_SEMI)){
        if (check(p, TOK_FSTRING)) {
            // f"name: {name}"
            Token ft = advance(p);
            arg = newNode(NODE_FSTRING);
            arg->text = _strdup(ft.text);
        }
        else if (check(p, TOK_STRING)) {
            //print a;
            Token str = advance(p);
            arg = newNode(NODE_STRING);
            arg->text = _strdup(str.text);
        }
        else {
            arg = parseExpr(p);
        }

        if (arg) {
            n->args = realloc(n->args, sizeof(Node *) * (n->argCount + 1));
            n->args[n->argCount++] = arg;
        }
    }

    expect(p, TOK_SEMI, ";");
    return n;
}

// `this you can comment
Node *parseIgnor(Parser *p) {
    int commentLine = peek(p).line;
    advance(p);  // consume `

    Node *n = newNode(NODE_IGNOR);

    //im allocating memory for an infinite number of letters in the comments.
    size_t cap = 64;
    n->text = malloc(1);
    n->text[0] = '\0';  

    // consume all tokens on the same line
    while (!check(p, TOK_EOF) && peek(p).line == commentLine) {
        size_t need = strlen(n->text) + strlen(peek(p).text) + 2;

        //i thought about making the comments go into C, but i gave up on it.
        if (need >= cap) {
            while(cap <= need) cap *= 2;
            n->text = realloc(n->text, cap);
        }

        strcat(n->text, peek(p).text);
        strcat(n->text, " ");
        
        advance(p);
    }

    return n;
}

Node *parseArray(Parser *p) {
    expect(p, TOK_LBRACKET, "[");
    Node *n = newNode(NODE_ARRAY);
    n->args = NULL;
    n->argCount = 0;

    if (!check(p, TOK_RBRACKET)) {
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

    expect(p, TOK_RBRACKET, "]");
    return n;
}

// for (expr) {}
Node *parseFor(Parser *p) {
    expect(p, TOK_KW_FOR, "for");
    expect(p, TOK_LPAREN, "(");

    Node *n = newNode(NODE_FOR);

    // for (i, times)
    if (check(p, TOK_TEXT) && p->tokens->tokens[p->pos + 1].type == TOK_COMMA) {

        Token var = advance(p);

        n->text = _strdup(var.text);
        expect(p, TOK_COMMA, ",");

        n->left = parseExpr(p);
    }
    // for(times)
    else {
        n->text = malloc(2);
        strcpy(n->text, "i");

        n->left = parseExpr(p);
    }

    expect(p, TOK_RPAREN, ")");

    n->thenBranch = parseBlock(p);

    return n;
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
DataType parseTypeAnnotation(Parser *p) {
    Token t = advance(p);
    switch (t.type) {
        case TOK_KW_INT:    return TYPE_INT;
        case TOK_KW_FLOAT:  return TYPE_FLOAT;
        case TOK_KW_STRING: return TYPE_STRING;
        case TOK_KW_BOOL:   return TYPE_BOOL;
        case TOK_KW_VOID:   return TYPE_VOID;
        default:
            printf("Parse error: expected type, got '%s' on line %d\n",
                   t.text, t.line);
            exit(1);
    }
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