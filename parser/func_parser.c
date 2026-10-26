#include "parser.h"

// Element type of the last parsed annotation (array<int> → int).
static DataType lastElementType;

// func name(params) -> type { ... }
Node *parseFuncDecl(Parser *p) {
    expect(p, TOK_KW_FUNC, "func");
    Token name = expect(p, TOK_TEXT, "name func");
    Node *n = newNode(NODE_FUNC_DECL);
    n->text = _strdup(name.text);
    n->paramCount = 0;
    n->returnType = TYPE_VOID;

    expect(p, TOK_LPAREN, "(");
    if (!check(p, TOK_RPAREN)) {
        while (1) {
            DataType paramType = parseTypeAnnotation(p);
            DataType paramElementType = lastElementType;
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
            n->params[n->paramCount].elementType = paramElementType;
            n->params[n->paramCount].name = _strdup(param.text);
            n->paramCount++;

            if (!check(p, TOK_COMMA)) break;
            advance(p);
        }
    }
    expect(p, TOK_RPAREN, ")");

    if (check(p, TOK_ARROW)) {
        advance(p);  // '->'
        n->returnType = parseTypeAnnotation(p);
        n->returnElementType = lastElementType;
    }

    n->thenBranch = parseBlock(p); // block func
    return n;
}

DataType parseTypeAnnotation(Parser *p) {
    Token t = advance(p);
    DataType base = TYPE_INT;

    switch (t.type) {
        case TOK_KW_INT:    base = TYPE_INT; break;
        case TOK_KW_FLOAT:  base = TYPE_FLOAT; break;
        case TOK_KW_STRING: base = TYPE_STRING; break;
        case TOK_KW_BOOL:   base = TYPE_BOOL; break;
        case TOK_KW_VOID:   base = TYPE_VOID; break;
        case TOK_KW_ARRAY: {
            DataType elementType = TYPE_INT;

            // array<string>, array<float>
            if (check(p, TOK_LT)) {
                advance(p); // '<'
                parseTypeAnnotation(p);
                elementType = lastElementType;
                if (check(p, TOK_GT)) advance(p); // '>'
            }

            lastElementType = elementType;
            return TYPE_ARRAY;
        }
        case TOK_KW_PTR: {
            DataType elementType = TYPE_INT;

            if (check(p, TOK_LT)) {
                advance(p); // '<'
                parseTypeAnnotation(p);
                elementType = lastElementType;
                if (check(p, TOK_GT)) advance(p); // '>'
            }

            lastElementType = elementType;
            return TYPE_POINTER;
        }
        default:
            exit(1);
    }

    // C-like pointer suffix: int*, float*, void*
    if (check(p, TOK_STAR)) {
        advance(p);
        lastElementType = base;
        return TYPE_POINTER;
    }

    lastElementType = base;
    return base;
}