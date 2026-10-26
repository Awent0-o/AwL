#include "parser.h"

// factor := NUMBER | TEXT | '(' expr ')'
Node *parseFactor(Parser *p) {
    Token t = peek(p);

    if (t.type == TOK_NOT) {
        advance(p);
        Node *n = newNode(NODE_NOT);
        n->left = parseFactor(p);
        return n;
    }

    if (t.type == TOK_AMP) {
        advance(p);
        Node *n = newNode(NODE_ADDR);
        n->left = parseFactor(p);
        return n;
    }

    if (t.type == TOK_STAR) {
        advance(p);
        Node *n = newNode(NODE_DEREF);
        n->left = parseFactor(p);
        return n;
    }

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
            n->line = t.line;
            n->text = _strdup(t.text);
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
        n->line = t.line;
        n->text = _strdup(t.text);
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
    return parseLogicalOr(p);
}

// logical OR: a || b
Node *parseLogicalOr(Parser *p) {
    Node *left = parseLogicalAnd(p);
    while (check(p, TOK_OR)) {
        Token op = advance(p);
        Node *right = parseLogicalAnd(p);
        Node *n = newNode(NODE_BINOP);
        n->op = op.type;
        n->left = left;
        n->right = right;
        left = n;
    }
    return left;
}

// logical AND: a && b
Node *parseLogicalAnd(Parser *p) {
    Node *left = parseComparison(p);
    while (check(p, TOK_AND)) {
        Token op = advance(p);
        Node *right = parseComparison(p);
        Node *n = newNode(NODE_BINOP);
        n->op = op.type;
        n->left = left;
        n->right = right;
        left = n;
    }
    return left;
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