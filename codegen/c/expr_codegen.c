#include "codegen_internal.h"

// genFString — gen f-string
// in:  f"name: {name}, age: {age}\n"
// out: "name: %s, age: %d\n", name, age
void genFString(const char *template, FILE *out) {
    //gen format string
    fprintf(out, "\"");
    const char *p = template;
    while (*p) {
        if (*p == '{') {
            //find {varName}
            p++; // skip '{'
            char varName[64] = {0};
            int i = 0;
            while (*p && *p != '}' && i < 63) {
                varName[i++] = *p++;
            }
            if (*p == '}') p++; //skip '}'

            // we determine the variable type for the correct format.
            DataType t = getVarType(varName);
            switch (t) {
                case TYPE_FLOAT:  fprintf(out, "%%f"); break;
                case TYPE_STRING: fprintf(out, "%%s"); break;
                case TYPE_BOOL:   fprintf(out, "%%d"); break;
                default:          fprintf(out, "%%d"); break;
            }
        } else if (*p == '\\' && *(p+1) == 'n') {
            fprintf(out, "\n");
            p += 2;
        } else if (*p == '\\' && *(p+1) == 't') {
            fprintf(out, "\t");
            p += 2;
        } else {
            // ordinary symbol
            if (*p == '"') fprintf(out, "\\\""); //escaping quotation marks
            else fputc(*p, out);
            p++;
        }
    }
    fprintf(out, "\"");

    //now we generate the arguments (iterating through the template again).
    p = template;
    while (*p) {
        if (*p == '{') {
            p++;
            char varName[64] = {0};
            int i = 0;
            while (*p && *p != '}' && i < 63) {
                varName[i++] = *p++;
            }
            if (*p == '}') p++;
            fprintf(out, ", %s", varName);
        } else {
            p++;
        }
    }
}

// Precedence used to preserve grouping when emitting C expressions.
static int binOpPrecedence(TokenType op) {
    switch (op) {
        case TOK_OR:    return 1;
        case TOK_AND:   return 2;
        case TOK_EQ:
        case TOK_NE:
        case TOK_LT:
        case TOK_GT:
        case TOK_LE:
        case TOK_GE:    return 3;
        case TOK_PLUS:
        case TOK_MINUS: return 4;
        case TOK_STAR:
        case TOK_SLASH: return 5;
        case TOK_INC:
        case TOK_DEC:   return 6;
        default:        return 7;
    }
}

void genExpr(Node *n, FILE *out) {
    if (!n) {
        fprintf(out, "0 /* ERROR: NULL expression */");
        return;
    }

    switch (n->type) {
        case NODE_NUMBER:
            fprintf(out, "%d", n->number);
            break;

        case NODE_TEXT:
            fprintf(out, "%s", n->text);
            break;

        case NODE_STRING:
            fprintf(out, "\"%s\"", n->text);
            break;

        case NODE_RAW_C:
            // raw C expression/statement: @{ ... }
            fprintf(out, "%s", n->text);
            break;

        case NODE_FLOAT:
            fprintf(out, "%f", n->fvalue);
            break;

        case NODE_BOOL:
            fprintf(out, "%s", n->number ? "true" : "false");
            break;

        case NODE_ADDR:
            fprintf(out, "&");
            if (n->left && n->left->type == NODE_TEXT) {
                genExpr(n->left, out);
            } else {
                fprintf(out, "(");
                genExpr(n->left, out);
                fprintf(out, ")");
            }
            break;

        case NODE_DEREF:
            fprintf(out, "*");
            if (n->left && n->left->type == NODE_TEXT) {
                genExpr(n->left, out);
            } else {
                fprintf(out, "(");
                genExpr(n->left, out);
                fprintf(out, ")");
            }
            break;

        case NODE_NOT:
            fprintf(out, "!(");
            genExpr(n->left, out);
            fprintf(out, ")");
            break;

        case NODE_INDEX:
            fprintf(out, "%s[", n->text);
            genExpr(n->left, out);
            fprintf(out, "]");
            break;

        case NODE_ARRAY:
            fprintf(out, "{");

            for (int i = 0; i < n->argCount; i++) {
                if (i) fprintf(out, ", ");
                genExpr(n->args[i], out);
            }

            fprintf(out, "}");
            break;

        case NODE_BINOP: {
            const char *opStr =
                n->op == TOK_PLUS  ? "+"  :
                n->op == TOK_MINUS ? "-"  :
                n->op == TOK_STAR  ? "*"  :
                n->op == TOK_SLASH ? "/"  :
                n->op == TOK_INC   ? "++" :
                n->op == TOK_DEC   ? "--" :
                n->op == TOK_AND   ? "&&" :
                n->op == TOK_OR    ? "||" :
                "?";

            int prec = binOpPrecedence(n->op);

            //parentheses when a child binds weaker, or when grouping matters on the right
            bool leftNeedsParens = n->left && n->left->type == NODE_BINOP &&
                            binOpPrecedence(n->left->op) < prec;

            bool rightNeedsParens = n->right && n->right->type == NODE_BINOP &&
                            (binOpPrecedence(n->right->op) < prec ||
                             (binOpPrecedence(n->right->op) == prec &&
                              (n->op == TOK_MINUS || n->op == TOK_SLASH ||
                               n->op == TOK_STAR)));

            if (leftNeedsParens)  fprintf(out, "(");
                genExpr(n->left, out);
            if (leftNeedsParens)  fprintf(out, ")");

                fprintf(out, " %s ", opStr);

            if (rightNeedsParens) fprintf(out, "(");
                if (n->right) genExpr(n->right, out);
            if (rightNeedsParens) fprintf(out, ")");
            break;
        }
        case NODE_COMPARE: {
            const char *opStr =
                n->op == TOK_LT ? "<" : n->op == TOK_GT ? ">" :
                n->op == TOK_LE ? "<=" : n->op == TOK_GE ? ">=" :
                n->op == TOK_EQ ? "==" : "!=";
                genExpr(n->left, out);
                fprintf(out, " %s ", opStr);
                genExpr(n->right, out);
                break;
        }
        case NODE_CALL: {
            //func from user
            fprintf(out, "%s(", n->text);
            for (int i = 0; i < n->argCount; i++) {
                if (i > 0) fprintf(out, ", ");
                genExpr(n->args[i], out);
            }
            fprintf(out, ")");
            break;
        }

        case NODE_PY_CALL: {
            //Py call func, i divided this case with NODE_CALL because, within he trown seg fail or not work
            PyFunc *pyf = findPyFunc(n->module, n->text);

            if (pyf) {
                //we select the correct _py_call_* based on the return type
                switch (pyf->returnType) {
                    case TYPE_STRING:
                        fprintf(out,
                        "_py_call_str(\"%s\", \"%s\", %d",
                        n->module,
                        n->text,
                        n->argCount);
                        break;
                    case TYPE_FLOAT:
                        fprintf(out,
                        "_py_call_float(\"%s\", \"%s\", %d",
                        n->module,
                        n->text,
                        n->argCount);
                        break;
                    case TYPE_BOOL:
                        fprintf(out,
                        "_py_call_int(\"%s\", \"%s\", %d",
                        n->module,
                        n->text,
                        n->argCount);
                        break;
                    default: // TYPE_INT
                        fprintf(out,
                        "_py_call_int(\"%s\", \"%s\", %d",
                        n->module,
                        n->text,
                        n->argCount);
                        break;
                }

                //arguments
                for (int i = 0; i < n->argCount; i++) {
                    fprintf(out, ", ");
                    genExpr(n->args[i], out);
                }
                fprintf(out, ")");
                break;
            }
            break;
        }

        default:
            fprintf(out, "0 /* ERROR: Unknown expression type %d */", n->type);
            break;
    }
}
