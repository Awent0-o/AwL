#include "codegen_internal.h"

//FUNC CODEGEN
/*
* func for genBlock, we save the scope of declared vars, and after the block we restore it
* FOR, WHILE, IF, BLOCK, FUNC DECLARE he have their own scope,
* so we need to save and restore the scope of declared vars
*/
void genBlock(Node *n, FILE *out) {
    if (!n) return;
    int savedCount = declaredCount;  //save scope

    fprintf(out, "{\n");
    for (int i = 0; i < n->stmtCount; i++) {
        genStmt(n->statements[i], out);
    }
    fprintf(out, "    }\n");

    declaredCount = savedCount;  //restore scope
}

void genStmt(Node *n, FILE *out) {
    if (!n) return;

    switch (n->type) {

        case NODE_ASSIGN:{
            fprintf(out, "    ");
            DataType extType = getVarType(n->text);

            if (n->left && n->left->type == NODE_CALL) {
                // find func in declaredVars
                Variable *func = getVar(n->left->text);
                DataType extType = getVarType(n->text);

                DataType retType = func ? func->type : TYPE_INT;
                // take element type from Variable
                DataType retElemType = (func &&
                           (func->type == TYPE_ARRAY || func->type == TYPE_POINTER))
                           ? func->elementType
                           : TYPE_INT;

                if (extType == TYPE_UNKNOWN) {
                    if(retType == TYPE_ARRAY || retType == TYPE_POINTER){
                        fprintf(out, "%s *%s = ", getTypeStr(retElemType), n->text);
                        regVar(n->text, retType, retElemType);
                    }
                    else {
                        fprintf(out, "%s %s = ", getTypeStr(retType), n->text);
                        regVar(n->text, retType, TYPE_UNKNOWN);
                    }
                    genExpr(n->left, out);
                    fprintf(out, ";\n");
                    break;
                }
                else {
                    fprintf(out, "    %s = ", n->text);
                }
                genExpr(n->left, out);
                fprintf(out, ";\n");
                break;
            }

            //rewrite because problem a = 2; -> int a = 2;
            //a = 1; -> int a = 1; when you need to reassign rather than recreate
            if (extType == TYPE_UNKNOWN) {
                DataType newType = ExprType(n->left);
                if (newType == TYPE_POINTER || newType == TYPE_ARRAY) {
                    DataType elementType = ExprElementType(n->left);
                    fprintf(out, "%s *%s = ", getTypeStr(elementType), n->text);
                    regVar(n->text, TYPE_POINTER, elementType);
                } else {
                    fprintf(out, "%s %s = ", getTypeStr(newType), n->text);
                    regVar(n->text, newType, TYPE_UNKNOWN);
                }
            }
            else {
                fprintf(out, "%s = ", n->text);
            }
            genExpr(n->left, out);
            fprintf(out, ";\n");
            break;
        }

        //rewritten from scratch to support f-strings and `print expr expr`.
        case NODE_PRINT: {
            if (n->argCount == 0) {
                fprintf(out, "    printf(\"\n\");\n");
                break;
            }

            // creat one printf with all arg
            fprintf(out, "    printf(");

            //string foramt
            fprintf(out, "\"");
            for (int i = 0; i < n->argCount; i++) {
                Node *arg = n->args[i];

                if (arg->type == NODE_STRING) {
                    // "hello world" — output as-is
                    // escape special characters
                    for (const char *c = arg->text; *c; c++) {
                        if (*c == '"')       fprintf(out, "\\\"");
                        else if (*c == '\\') fprintf(out, "\\");
                        else                 fputc(*c, out);
                    }
                }
                else if (arg->type == NODE_FSTRING) {
                    // f"text {var} text" — replace {var} on format
                    const char *p = arg->text;
                    while (*p) {
                        if (*p == '{') {
                            p++;
                            char varName[64] = {0};
                            int j = 0;
                            while (*p && *p != '}' && j < 63) varName[j++] = *p++;
                            if (*p == '}') p++;

                            DataType t = getVarType(varName);
                            fprintf(out, "%s", getFormatSpec(t));
                        }
                        else if (*p == '\\' && *(p + 1)) {
                            fputc('\\', out);
                            p++;
                            fputc(*p, out);
                            p++;
                        }
                        else {
                            if (*p == '"') fprintf(out, "\\\"");
                            else fputc(*p, out);
                            p++;
                        }
                    }
                }
                else if (arg->type == NODE_ARRAY)
                {
                    DataType t = ExprType(arg);
                    t = arg->returnElementType;
                    fprintf(out, "%s", getFormatSpec(t));
                }

                else {
                    // var or format
                    DataType t = ExprType(arg);
                    fprintf(out, "%s", getFormatSpec(t));
                }
            }
            fprintf(out, "\"");

            // arguments
            for (int i = 0; i < n->argCount; i++) {
                Node *arg = n->args[i];

                if (arg->type == NODE_STRING) {
                    // string literal — not needed as a printf argument
                    // (already embedded in the format string)
                    continue;
                } else if (arg->type == NODE_FSTRING) {
                    // extract variables from f-string
                    const char *p = arg->text;
                    while (*p) {
                        if (*p == '{') {
                            p++;
                            char varName[64] = {0};
                            int j = 0;
                            while (*p && *p != '}' && j < 63) varName[j++] = *p++;
                            if (*p == '}') p++;
                            fprintf(out, ", %s", varName);
                        } else {
                            p++;
                        }
                    }
                } else {
                    // expr
                    fprintf(out, ", ");
                    genExpr(arg, out);
                }
            }

            fprintf(out, ");\n");
            break;
        }

        case NODE_IF:
            fprintf(out, "if( ");
            genExpr(n->cond, out);
            fprintf(out, " )");
            genBlock(n->thenBranch, out);
            if (n->elseBranch) {
                fprintf(out, "    else ");
                genBlock(n->elseBranch, out);
            }
            break;

        case NODE_WHILE:
            fprintf(out, "while( ");
            genExpr(n->cond, out);
            fprintf(out, " )");
            genBlock(n->thenBranch, out);
            break;

        case NODE_BLOCK:
            fprintf(out, "{\n");
            for (int i = 0; i < n->stmtCount; i++) {
                genBlock(n->statements[i], out);
            }
            fprintf(out, "    }\n");
            break;

        case NODE_FOR:
            fprintf(out, "for(int %s =  ", n->text);
            if (n->right) {
                // for(i = 0, times) → for(int i = 0; i < times; i++)
                genExpr(n->left, out);          // start
                fprintf(out, "; %s < ", n->text);
                genExpr(n->right, out);         // end
            }
            else {
                // for(i, times) or for(times) → for(int i = 0; i < times; i++)
                fprintf(out, "0; %s < ", n->text);
                genExpr(n->left, out);          // end
            }
            fprintf(out, "; %s++) ", n->text);
            genBlock(n->thenBranch, out);
            break;

        //rewritten from scratch to support return arr and use arr in params
        case NODE_RETURN: {
            //ARRAY RETURN
            if (n->expr && n->expr->type == NODE_ARRAY) {
                // return static buf
                fprintf(out, "    {\n");
                fprintf(out, "        int _tmp[] = ");
                genExpr(n->expr, out);
                fprintf(out, ";\n");
                fprintf(out, "        memcpy(_awl_ret_buf, _tmp, sizeof(_tmp));\n");
                fprintf(out, "        return _awl_ret_buf;\n");
                fprintf(out, "    }\n");
            }
            //VAR RETURN
            else if (n->expr && n->expr->type == NODE_TEXT) {
                // name arr
                DataType t = getVarType(n->expr->text);
                if (t == TYPE_ARRAY) {
                    fprintf(out, "    return %s;\n", n->expr->text);
                }
                else {
                fprintf(out, "    return ");
                genExpr(n->expr, out);
                fprintf(out, ";\n");
                }
            }
            //DEFUALT RETURN
            else {
                fprintf(out, "    return ");
                genExpr(n->expr, out);
                fprintf(out, ";\n");
            }
            break;
        }

        case NODE_EXPR_STMT:
            fprintf(out, "    ");
            genExpr(n->expr, out);
            fprintf(out, ";\n");
            break;

        case NODE_RAW_C:
            fprintf(out, "%s\n", n->text);
            break;

        case NODE_ARRAY: {
            // save arguments details from array
            DataType elementType = ExprType(n->left->args[0]);
            //                         ↓translete type in str
            fprintf(out, "%s %s[] = ", getTypeStr(elementType), n->text);
            genExpr(n->left, out);
            fprintf(out, ";\n");
            regVar(n->text, TYPE_ARRAY, elementType);
            break;
        }

        case NODE_INDEX_ASSIGN:
            fprintf(out, "    %s[", n->text);
            genExpr(n->left, out);   // index
            fprintf(out, "] = ");
            genExpr(n->right, out);  // value
            fprintf(out, ";\n");
            break;

        case NODE_DEREF_ASSIGN:
            fprintf(out, "    ");
            genExpr(n->left, out);   // *p
            fprintf(out, " = ");
            genExpr(n->right, out);  // value
            fprintf(out, ";\n");
            break;

        default:
            break;
    }
}

void genFuncDecl(Node *n, FILE *out) {
    //we are creating a visibility zone.
    int savedCount = declaredCount;

    //we a save var earlier than everyone else
    regVar(n->text, n->returnType, n->returnElementType);

    if (n->returnType == TYPE_ARRAY || n->returnType == TYPE_POINTER) {
        fprintf(out, "%s *%s(", getTypeStr(n->returnElementType), n->text);
    } else {
        fprintf(out, "%s %s(", getTypeStr(n->returnType), n->text);
    }

    for (int i = 0; i < n->paramCount; i++) {
        if (i > 0) fprintf(out, ", ");
        if(n->params[i].type == TYPE_ARRAY || n->params[i].type == TYPE_POINTER){
            // array<int> / int* → int *arr
            fprintf(out, "%s *%s",
                    getTypeStr(n->params[i].elementType),
                    n->params[i].name);
        }
        else{
            fprintf(out, "%s %s", getTypeStr(n->params[i].type), n->params[i].name);
        }
    }
    fprintf(out, ") {\n");

    //reg all params in declaredVars for use in f-string and print
    for (int i = 0; i < n->paramCount; i++) {
        regVar(n->params[i].name,
               n->params[i].type,
               n->params[i].elementType);
    }

    if (!n->thenBranch){
        fprintf(out, "}\n\n");
        declaredCount = savedCount;
        return;
    }

    for (int i = 0; i < n->thenBranch->stmtCount; i++) {
        genStmt(n->thenBranch->statements[i], out);
    }

    fprintf(out, "}\n\n");
    declaredCount = savedCount;
}

static void genTopLevelStmt(Node *stmt, FILE *out) {
    if (!stmt) return;
    if (stmt->type == NODE_IMPORT || stmt->type == NODE_FUNC_DECL) return;
    genStmt(stmt, out);
}

void genMainEntry(Node *ast, Node *entry, FILE *out) {
    fprintf(out, "int main(");
    genMainParams(entry, out);
    fprintf(out, ") {\n");

    // register main params so print/f-string can infer argc (%d) and
    // argv[i] (%s for array<string>) types inside the body
    if (entry) {
        for (int i = 0; i < entry->paramCount; i++) {
            regVar(entry->params[i].name,
                   entry->params[i].type,
                   entry->params[i].elementType);
        }
    }

    for (int i = 0; i < pyFileCount; i++) {
        // "lib.py" -> modname = "lib"
        char modname[64] = {0};
        const char *path = pyFiles[i];
        const char *slash = strrchr(path, '/');
        const char *base = slash ? slash + 1 : path;
        int len = strlen(base);
        if (len > 3 && strcmp(base + len - 3, ".py") == 0) {
            strncpy(modname, base, len - 3);
        } else {
            strncpy(modname, base, 63);
        }
        fprintf(out, "    _py_load(\"%s\", \"%s\");\n", path, modname);
    }

    // top-level code runs at the start of main (before a user main body)
    for (int i = 0; i < ast->stmtCount; i++) {
        genTopLevelStmt(ast->statements[i], out);
    }

    if (entry && entry->thenBranch) {
        for (int i = 0; i < entry->thenBranch->stmtCount; i++) {
            genStmt(entry->thenBranch->statements[i], out);
        }
    }

    fprintf(out, "    return 0;\n}\n");
}
