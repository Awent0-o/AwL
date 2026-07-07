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

Variable *getVar(const char *name) {
    for (int i = 0; i < declaredCount; i++) {
        if (strcmp(declaredVars[i].name, name) == 0)
            return &declaredVars[i];
    }
    return NULL;
}

// Зареєструвати змінну з її типом
void regVar(const char *name, DataType type, DataType elementType) {
    strcpy(declaredVars[declaredCount].name, name);
    declaredVars[declaredCount].type = type;
    declaredVars[declaredCount].elementType = elementType;
    declaredCount++;
}

const char *getTypeStr(DataType type) {
    switch (type) {
        case TYPE_INT: return "int";
        case TYPE_STRING: return "const char*";
        case TYPE_FLOAT: return "float";
        case TYPE_BOOL: return "bool";
        default: return "unknown";
    }
}

DataType ExprType(Node *n) {
    if (!n) return TYPE_INT;

    switch (n->type) {
        case NODE_NUMBER: return TYPE_INT;
        case NODE_STRING: return TYPE_STRING;            
        case NODE_FLOAT:  return TYPE_FLOAT;
        case NODE_BOOL:   return TYPE_BOOL;     
        
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

        case NODE_ARRAY_LITERAL:
            return TYPE_ARRAY;

        default: return TYPE_INT;
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

        case NODE_FLOAT:
            fprintf(out, "%f", n->fvalue);
            break;

        case NODE_BOOL:
            fprintf(out, "%s", n->number ? "true" : "false");
            break;

        case NODE_INDEX:
            fprintf(out, "%s[", n->text);
            genExpr(n->left, out);
            fprintf(out, "]");
            break;       

        case NODE_ARRAY_LITERAL:
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
                "?";
            
            genExpr(n->left, out);
            fprintf(out, " %s ", opStr);
            if(n->right != NULL) genExpr(n->right, out);      
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
        fprintf(out, "0 /* ERROR: Unknown expression type %d */", n->type);
        break;
    }
}

void genStmt(Node *n, FILE *out) {
    if (!n) return;
    
    switch (n->type) {
            
        case NODE_ASSIGN:{
            fprintf(out, "    ");
            DataType extType = getVarType(n->text);
            
            if(n->left->type == NODE_ARRAY_LITERAL){
                DataType elementType = ExprType(n->left->args[0]);
                printf("in array %d\n", elementType);
                
                fprintf(out, "%s %s[] = ", getTypeStr(elementType), n->text);
                genExpr(n->left, out);
                fprintf(out, ";\n");
                regVar(n->text, TYPE_ARRAY, elementType);

                break;

            }

            else if(extType == TYPE_UNKNOWN){

                DataType newType = ExprType(n->left);

                const char *typeStr = getTypeStr(newType);
                fprintf(out, "%s %s = ", typeStr, n->text);
                genExpr(n->left, out);
                fprintf(out, ";\n");
                regVar(n->text, newType, 0);

                break;
                
            }
            else{
                fprintf(out, "%s ", getTypeStr(extType));
                fprintf(out, " %s = ", n->text);
                genExpr(n->left, out);
                fprintf(out, ";\n");

            }
            break;
        }
            
            
        case NODE_PRINT:{
                
            DataType printType = TYPE_INT; 
            Variable *var = getVar(n->expr->text);

            if (var == NULL) {
                printf("Variable '%s' not found\n", n->expr->text);
                break;
            }

            printf("elementType = %d\n", var->elementType);

            if(n->expr){
                if(n->expr->type == NODE_STRING) printType = TYPE_STRING;
                else if(n->expr->type == NODE_INDEX) printType = TYPE_ARRAY;
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
                else if (printType == TYPE_ARRAY) {
                    printf("printType = %d TYPE_ARRAY = %d\n", printType, TYPE_ARRAY);
                    printf("element type = %d\n", var->elementType);

                    switch (var->elementType) {            
                        case TYPE_STRING:
                            fprintf(out, "printf(\"%%s\\n\", ");
                            break;
                    
                        case TYPE_FLOAT:
                            fprintf(out, "printf(\"%%f\\n\", ");
                            break;

                        default:
                            fprintf(out, "printf(\"%%d\\n\", ");
                            break;

                    }
                
                    genExpr(n->expr, out);
                    fprintf(out, ");\n");
                }
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
        
        case NODE_RAW_C:
            fprintf(out, "%s\n", n->text);
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

    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <stdbool.h>\n");
    // fprintf(out, "#include <%s>\n\n", n->text);

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