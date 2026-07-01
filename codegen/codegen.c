#include "codegen.h"

#define MAX_VARS 256
static Variable declaredVars[MAX_VARS];
static int declaredCount = 0;

// Отримати тип змінної за її іменем
DataType getVarType(const char *name) {
    for (int i = 0; i < declaredCount; i++) {
        if (strcmp(declaredVars[i].name, name) == 0) return declaredVars[i].type;
    }
    return TYPE_UNKNOWN;
}

// Зареєструвати змінну з її типом
void regVar(const char *name, DataType type, int arraySize) {
    strcpy(declaredVars[declaredCount].name, name);
    declaredVars[declaredCount].type = type;
    declaredVars[declaredCount].arraySize = arraySize;
    declaredCount++;
}

DataType ExprType(Node *n) {
    if (!n) return TYPE_INT;

    switch (n->type) {
        case NODE_NUMBER: return TYPE_INT;
        case NODE_STRING: return TYPE_STRING;            
        case NODE_FLOAT:  return TYPE_FLOAT;
        //case NODE_ARRAY:  return TYPE_INT_ARRAY;        
        
        case NODE_TEXT: {
            // var = var2
            DataType t = getVarType(n->text);
            return (t != TYPE_UNKNOWN) ? t : TYPE_INT;
        }
        case NODE_BINOP: {
            // if one operand float, all do float
            if (ExprType(n->left) == TYPE_FLOAT || 
                ExprType(n->right) == TYPE_FLOAT) {
                return TYPE_FLOAT;
            }
            return TYPE_INT;
        }
        default: return TYPE_INT;
    }
}

void genExpr(Node *n, FILE *out) {
    
    if (!n) {
        fprintf(out, "0 /* ERROR: NULL expression */");
        return;
    }

    printf("genExpr: type=%d\n", n->type);
        
    switch (n->type) {
        case NODE_NUMBER:
            fprintf(out, "%d", n->number);
            break;

        case NODE_TEXT:                 //<- idk why this not work together with NODE_STRING
            fprintf(out, "%s", n->text);
            break;

        case NODE_STRING:
            fprintf(out, "\"%s\"", n->text);
            break;

        case NODE_FLOAT:
            fprintf(out, "%f", n->fvalue);
            break;

        case NODE_BOOL:
            fprintf(out, "%s", n->number ? "true" : "false");
            break;

        case NODE_INDEX:
            fprintf(out, "%s[", n->text);
            genExpr(n->left, out); // index
            fprintf(out, "]");
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
                genExpr(n->left, out);
                fprintf(out, " %s ", opStr);
                genExpr(n->right, out);
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
            fprintf(out, "0 /* ERROR: Unknown expression type %d */", n->type);
            break;
        }
}

void genStmt(Node *n, FILE *out) {
    if (!n) return;

    printf("genStmt: type=%d\n", n->type);

    switch (n->type) {
        case NODE_IGNOR:
            fprintf(out, "//%s", n->text);
            break;

        case NODE_ASSIGN:{
            fprintf(out, "    ");
            DataType extType = getVarType(n->text);

            if(extType == TYPE_UNKNOWN){

                DataType newType = ExprType(n->left);

                if(newType == TYPE_INT){
                    fprintf(out, "int ");
                    regVar(n->text, TYPE_INT, 0);
                }
                else if(newType == TYPE_STRING){
                    fprintf(out, "const char* ");
                    regVar(n->text, TYPE_STRING, 0);
                }
                else if(newType == TYPE_FLOAT){
                    fprintf(out, "float ");
                    regVar(n->text, TYPE_FLOAT, 0);
                }
                else if(newType == TYPE_BOOL){
                    fprintf(out, "bool ");
                    regVar(n->text, TYPE_BOOL, 0);
                }
                //else if(newType == TYPE_INT_ARRAY){
                //just only use int, next version do all
                //    fprintf(out, "int %s[]", n->text);

                //n->left node array, in args lie down his elements
                //    if(n->left && n->left->type == NODE_ARRAY){
                //        for(int i = 0; i < n->left->argCount; i++){
                //            if(i > 0) fprintf(out, ", ");
                //            genExpr(n->left->args[i], out);
                //        }
                //        regVar(n->text, TYPE_INT_ARRAY, n->left->argCount);
                //    }
                //    fprintf(out, "};\n");
                //    break;
                //}
            }

            // стандартне присвоєння зміних
            fprintf(out, " %s = ", n->text);
            genExpr(n->left, out);
            fprintf(out, ";\n");

            break;
        }

        case NODE_PRINT:{
            
            DataType printType = TYPE_INT;

            if(n->expr){
                if(n->expr->type == NODE_STRING) printType = TYPE_STRING;
                else if(n->expr->type == NODE_FLOAT) printType = TYPE_FLOAT;
                else if(n->expr->type == NODE_TEXT){
                    DataType t = getVarType(n->expr->text);
                    if(t != TYPE_UNKNOWN) printType = t;
                    else printType = ExprType(n->expr);
                }

                //init type for print text
                if (printType == TYPE_STRING) {
                    fprintf(out, "    printf(\"%%s\\n\", ");
                    genExpr(n->expr, out);
                    fprintf(out, ");\n");
                } 
                else if (printType == TYPE_FLOAT) {
                    fprintf(out, "    printf(\"%%f\\n\", ");
                    genExpr(n->expr, out);
                    fprintf(out, ");\n");
                } 
                //else if (printType == TYPE_INT_ARRAY) {
                //    //and i working on print array, now that not work
                //    fprintf(out, "    printf(\"[Array Pointer: %%p]\\n\", %s);\n", n->expr->text);
                //} 
                else {
                    // int and bool
                    fprintf(out, "    printf(\"%%d\\n\", ");
                    genExpr(n->expr, out);
                    fprintf(out, ");\n");
                }
            }
            break;
        }

        case NODE_IF:
            fprintf(out, "    if( ");
            genExpr(n->cond, out);
            fprintf(out, " )");
            genStmt(n->thenBranch, out);
            if (n->elseBranch) {
                fprintf(out, "    else ");
                genStmt(n->elseBranch, out);
            }
            break;

        case NODE_WHILE:
            fprintf(out, "    while( ");
            genExpr(n->cond, out);
            fprintf(out, " )");
            genStmt(n->thenBranch, out);
            break;

        case NODE_BLOCK:
            fprintf(out, "{\n");
            for (int i = 0; i < n->stmtCount; i++) {
                genStmt(n->statements[i], out);
            }
            fprintf(out, "    }\n");
            break;

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
    fprintf(out, "}\n\n");
}


void genC(Node *ast, FILE *out) {

    if (!ast) { printf("genC: NULL ast!\n"); return; }
    printf("genC: stmtCount=%d\n", ast->stmtCount);

    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <stdbool.h>\n\n");

    for (int i = 0; i < ast->stmtCount; i++) {
        if (ast->statements[i]->type == NODE_FUNC_DECL) {
            genFuncDecl(ast->statements[i], out);
        }
    }

    fprintf(out, "int main() {\n");
    for (int i = 0; i < ast->stmtCount; i++) {
        if (ast->statements[i]->type != NODE_FUNC_DECL) {
            printf("genC: stmt[%d] type=%d\n", i, ast->statements[i]->type);
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