#include "parser.h"

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
    n->line = ident.line;
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
    n->line = peek(p).line;
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

// for (expr) {}
Node *parseFor(Parser *p) {
    expect(p, TOK_KW_FOR, "for");
    expect(p, TOK_LPAREN, "(");

    Node *n = newNode(NODE_FOR);

    // for (i = 0, times)
    if (check(p, TOK_TEXT) && p->tokens->tokens[p->pos + 1].type == TOK_ASSIGN) {

        Token var = advance(p);

        n->text = _strdup(var.text);
        advance(p);
        n->left  = parseExpr(p);
        expect(p, TOK_COMMA, ",");

        n->right = parseExpr(p);
    }
    // for(i, times);
    else if (check(p, TOK_TEXT) && p->tokens->tokens[p->pos + 1].type == TOK_COMMA) {

        Token var = advance(p);

        n->text = _strdup(var.text);
        advance(p);
        n->left  = parseExpr(p);
        n->right = NULL;
        expect(p, TOK_COMMA, ",");

        n->right = parseExpr(p);
    }

    // for(times)
    else {
        n->text = _strdup("i");
        n->left = parseExpr(p);
        n->right = NULL;
    }

    expect(p, TOK_RPAREN, ")");

    n->thenBranch = parseBlock(p);

    return n;
}
