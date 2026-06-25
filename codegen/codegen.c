#include "codegen.h"

#define MAX_VARS 256
static char declaredVars[MAX_VARS][64];
static int declaredCount = 0;

bool isDeclared(const char *name) {
    for (int i = 0; i < declaredCount; i++) {
        if (strcmp(declaredVars[i], name) == 0) return true;
    }
    return false;
}

void markDeclared(const char *name) {
    strcpy(declaredVars[declaredCount++], name);
}
void genExpr(Node *n, FILE *out) {
    switch (n->type) {
        case NODE_NUMBER:
            fprintf(out, "%d", n->number);
            break;
        case NODE_TEXT:
            fprintf(out, "%s", n->text);
            break;
        case NODE_BINOP: {
            const char *opStr = n->op == TOK_PLUS ? "+" :
                                 n->op == TOK_MINUS ? "-" :
                                 n->op == TOK_STAR ? "*" : "/";
            fprintf(out, "(");
            genExpr(n->left, out);
            fprintf(out, " %s ", opStr);
            genExpr(n->right, out);
            fprintf(out, ")");
            break;
        }
        case NODE_COMPARE: {
            const char *opStr =
                n->op == TOK_LT ? "<" : n->op == TOK_GT ? ">" :
                n->op == TOK_LE ? "<=" : n->op == TOK_GE ? ">=" :
                n->op == TOK_EQ ? "==" : "!=";
            fprintf(out, "(");
            genExpr(n->left, out);
            fprintf(out, " %s ", opStr);
            genExpr(n->right, out);
            fprintf(out, ")");
            break;
        }
        case NODE_CALL: {
            fprintf(out, "%s(", n->text);
            for (int i = 0; i < n->argCount; i++) {
                if (i > 0) fprintf(out, ", ");
                genExpr(n->args[i], out);
            }
            fprintf(out, ")");
            break;
        }
        default:
            break;
    }
}

void genStmt(Node *n, FILE *out) {
    switch (n->type) {
        // ... ASSIGN, PRINT, IF, WHILE, BLOCK ...

        case NODE_RETURN:
            fprintf(out, "    return ");
            genExpr(n->expr, out);
            fprintf(out, ";\n");
            break;

        case NODE_EXPR_STMT:
            fprintf(out, "    ");
            genExpr(n->expr, out);
            fprintf(out, ";\n");
            break;

        default:
            break;
    }
}

void genFuncDecl(Node *n, FILE *out) {
    fprintf(out, "int %s(", n->text);
    for (int i = 0; i < n->paramCount; i++) {
        if (i > 0) fprintf(out, ", ");
        fprintf(out, "int %s", n->params[i]);
    }
    fprintf(out, ") {\n");
    for (int i = 0; i < n->thenBranch->stmtCount; i++) {
        genStmt(n->thenBranch->statements[i], out);
    }
    fprintf(out, "    return 0;\n"); // return
    fprintf(out, "}\n\n");
}


void genC(Node *ast, FILE *out) {
    fprintf(out, "#include <stdio.h>\n\n");

    for (int i = 0; i < ast->stmtCount; i++) {
        if (ast->statements[i]->type == NODE_FUNC_DECL) {
            genFuncDecl(ast->statements[i], out);
        }
    }

    fprintf(out, "int main() {\n");
    for (int i = 0; i < ast->stmtCount; i++) {
        if (ast->statements[i]->type != NODE_FUNC_DECL) {
            genStmt(ast->statements[i], out);
        }
    }
    fprintf(out, "    return 0;\n}\n");
}

void genBin(const char *filename) {
    char compile_cmd[512];
    char remove_cmd[256];

    snprintf(compile_cmd, sizeof(compile_cmd), "gcc %s.c -o %s", filename, filename);

    snprintf(remove_cmd, sizeof(remove_cmd), "rm %s.c", filename);

    printf("Executing: %s\n", compile_cmd);
    int res = system(compile_cmd);

    if (res == 0) {
        printf("Executing: %s\n", remove_cmd);
        system(remove_cmd);
    } else {
        fprintf(stderr, "Compilation failed! Temporary .c file was preserved.\n");
    }
}
