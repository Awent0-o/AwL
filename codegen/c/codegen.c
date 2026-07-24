#include "codegen.h"

//VAR CODEGEN
#define MAX_VARS 256
static Variable declaredVars[MAX_VARS];
static int declaredCount = 0;
bool hasPython = false;

//PY VAR
static char pyFiles[16][256];
static int pyFileCount = 0;

// FUNC UTILS CODEGEN 
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
        case TYPE_VOID: return "void";
        case TYPE_AUTO: return "__auto_type";
        default: return "unknown";
    }
}

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
            // store varName for arguments (below) 
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

// getFormatSpec — return type for printf 
static const char *getFormatSpec(DataType t) {
    switch (t) {
        case TYPE_FLOAT:  return "%f";
        case TYPE_STRING: return "%s";
        case TYPE_BOOL:   return "%d";
        default:          return "%d";
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
            return (t != TYPE_UNKNOWN) ? t : TYPE_AUTO;
        }
        case NODE_BINOP: {
            // if one operand float, all do float
            if (ExprType(n->left) == TYPE_FLOAT || 
                ExprType(n->right) == TYPE_FLOAT) {
                return TYPE_FLOAT;
            }
            return TYPE_AUTO;
        }

        case NODE_ARRAY:
            return TYPE_ARRAY;

        case NODE_CALL:{
            return n->returnType;
        }

        case NODE_PY_CALL: {
            PyFunc *pyf = NULL;

            if (n->module) pyf = findPyFunc(n->module, n->text);
            if (pyf != NULL) {

                return pyf->returnType;
            }
            return TYPE_CALL;
        }
        
        default: return TYPE_AUTO;
    }
}

//FUNC CODEGEN
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

        case NODE_PY_CALL:
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

            if(n->left->type == NODE_ARRAY){
                DataType elementType = ExprType(n->left->args[0]);
                
                fprintf(out, "%s %s[] = ", getTypeStr(elementType), n->text);
                genExpr(n->left, out);
                fprintf(out, ";\n");
                regVar(n->text, TYPE_ARRAY, elementType);

                break;

            }
            //rewrite because problem a = 2; -> int a = 2;
            //a = 1; -> int a = 1; when you need to reassign rather than recreate
            if (extType == TYPE_UNKNOWN) {
                DataType newType = ExprType(n->left);
                fprintf(out, "%s %s = ", getTypeStr(newType), n->text);
                genExpr(n->left, out);
                fprintf(out, ";\n");
                regVar(n->text, newType, TYPE_UNKNOWN);
            }
            else {
                fprintf(out, "%s = ", n->text);
                genExpr(n->left, out);
                fprintf(out, ";\n");
            }
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
                
                } else if (arg->type == NODE_FSTRING) {
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
                            // actually, we need to write %s, %d — but we are inside a string
                            // so we write it like this:
                            // getFormatSpec returns "%d", but we need "%%d" in printf
                            // to avoid double formatting
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
                } else {
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
            genStmt(n->thenBranch, out);
            if (n->elseBranch) {
                fprintf(out, "    else ");
                genStmt(n->elseBranch, out);
            }
            break;

        case NODE_WHILE:
            fprintf(out, "while( ");
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
        
        case NODE_FOR:
            const char* name = n->text;
            fprintf(out, "for(int %s = 0; %s < ", name, name);
            genExpr(n->left, out);
            fprintf(out, "; %s++) ", n->text);
            genStmt(n->thenBranch, out);
            break;

        case NODE_RETURN:
            fprintf(out, "return ");
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
    int savedCount = declaredCount; 

    for (int i = 0; i < n->paramCount; i++) {
        regVar(n->params[i].name, n->params[i].type, 0);
    }

    fprintf(out, "%s %s(", getTypeStr(n->returnType), n->text);
    for (int i = 0; i < n->paramCount; i++) {
        if (i > 0) fprintf(out, ", ");
        fprintf(out, "%s %s", getTypeStr(n->params[i].type), n->params[i].name);
    }

    fprintf(out, ") {\n");

    if (!n->thenBranch){ 
        fprintf(out, "}\n\n"); 
        return; 
    }

    for (int i = 0; i < n->thenBranch->stmtCount; i++) {
        genStmt(n->thenBranch->statements[i], out);
    }
    
    fprintf(out, "}\n\n");
    declaredCount = savedCount; 
}

void genImport(Node *n, FILE *out) {
    const char *path = n->text;
    size_t len = strlen(path);

    //import ".py"
    if (len > 3 && strcmp(path + len - 3, ".py") == 0) {
        if (!hasPython) {
            
            hasPython = true;
            fprintf(out, "#include <stdarg.h>\n");
            fprintf(out, "%s", AWL_PY_RUNTIME);
        }
        strcpy(pyFiles[pyFileCount++], path);

        //parse file py and memorize func
        PyModule mod = parsePyFile(path);

        pyModules[pyModuleCount] = mod;
        snprintf(pyModuleNames[pyModuleCount], sizeof(pyModuleNames[pyModuleCount]), "%s", mod.name);
        pyModuleCount++;
        return;
    }

    //import ".awl"
    if (len > 4 && strcmp(path + len - 4, ".awl") == 0) {
        // parse awl
        size_t fileLen;
        char *src = readFile(path, &fileLen);
        if (!src) {
            fprintf(stderr, "Cannot open import: %s\n", path);
            return;
        }

        TokenList tokens = tokenize(src, fileLen);
        Parser parser = { .tokens = &tokens, .pos = 0 };
        Node *importedAst = parseProgram(&parser);

        // gen only func
        for (int i = 0; i < importedAst->stmtCount; i++) {
            if (importedAst->statements[i]->type == NODE_FUNC_DECL) {
                genFuncDecl(importedAst->statements[i], out);
            }
        }

        free(src);
        free(tokens.tokens);
        return;
    }

    // else - C lib
    // "math" -> #include <math.h>
    // "myheader.h" -> #include "myheader.h"
    if (len > 2 && strcmp(path + len - 2, ".h") == 0) {
        fprintf(out, "#include \"%s\"\n", path);
    } else {
        fprintf(out, "#include <%s.h>\n", path);
    }
}

void genC(Node *ast, FILE *out) {
    
    if (!ast) { printf("genC: NULL ast!\n"); return; }
    
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <stdbool.h>\n");
    fprintf(out, "%s\n", AUTOPRINTTYPE);
    
    for (int i = 0; i < ast->stmtCount; i++) {
        if (ast->statements[i]->type == NODE_IMPORT) {
            genImport(ast->statements[i], out);
        }
    }
    
    for (int i = 0; i < ast->stmtCount; i++) {
        if (ast->statements[i]->type == NODE_FUNC_DECL) {
            genFuncDecl(ast->statements[i], out);
        }
    }
    
    fprintf(out, "int main() {\n");
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
    
    for (int i = 0; i < ast->stmtCount; i++) {
        if (ast->statements[i]->type != NODE_FUNC_DECL) {
            genStmt(ast->statements[i], out);
        }
    }
    fprintf(out, "return 0;\n}\n");
}

void genBin(const char *filename) {
    char compile_cmd[2048];
    char flags[1024] = "";
    char remove_cmd[256];
    int res = 0;

    snprintf(compile_cmd, sizeof(compile_cmd), "gcc %s.c", filename);

    // Python flag
    if (hasPython) {
        strncat(flags, " $(python3-config --includes --ldflags --embed)",
                sizeof(flags) - strlen(flags) - 1);
    }

    //out file
    char out[256];
    snprintf(out, sizeof(out), " -o %s", filename);

    // full command
    strncat(compile_cmd, flags, sizeof(compile_cmd) - strlen(compile_cmd) - 1);
    strncat(compile_cmd, out, sizeof(compile_cmd) - strlen(compile_cmd) - 1);

    printf("Compiling: %s\n", compile_cmd);
    res = system(compile_cmd);
    if (res != 0) {
        fprintf(stderr, "Compilation failed!\n");
    }

    snprintf(remove_cmd, sizeof(remove_cmd), "rm %s.c", filename);

    printf("Executing: %s\n", compile_cmd);
    res = system(compile_cmd);

    if (res == 0) {
        printf("Executing: %s\n", remove_cmd);
        system(remove_cmd);
    } else {
        fprintf(stderr, "Compilation failed! Temporary .c file was preserved.\n");
    }
}