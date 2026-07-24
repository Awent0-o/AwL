#include "semantic/semantic.h"

// VAR FOR SEMANTIC
static SemResult result;

static Scope *currentScope = NULL;

static DataType currentFuncReturnType = TYPE_VOID;

bool hasCImport = false;

static char currentFuncName[64] = {0};

// FUNC SEMANTIC

static void addError(SemErrorType type, int line, bool fatal,
                     const char *fmt, ...) {
    if (result.count >= MAX_SEM_ERRORS) return;

    SemError *err = &result.errors[result.count++];
    err->type    = type;
    err->line    = line;
    err->isFatal = fatal;

    va_list args;
    va_start(args, fmt);
    vsnprintf(err->message, sizeof(err->message), fmt, args);
    va_end(args);

    if (fatal) result.hasErrors = true;
}


/* 
 * pushScope — creates a new scope and makes it the current one.
 * Called upon entering a { } block. 
*/
static void pushScope(void) {
    Scope *s = calloc(1, sizeof(Scope));
    s->parent    = currentScope;
    currentScope = s;
}


/* 
 * popScope — destroys the current scope and returns to the parent scope.
 * Checks for unused variables before destruction.
 * Called upon exiting a { } block. 
*/
static void popScope(void) {
    if (!currentScope) return;

    // check every variable in this scope
    for (int i = 0; i < currentScope->count; i++) {
        Symbol *sym = &currentScope->symbols[i];

        //we do not check functions and parameters for being unused
        if (sym->isFunc) continue;

        //issue a warning if the variable was not used
        if (!sym->used) {
            addError(SEM_WARN_UNUSED_VAR, sym->line, false,
                     "Warning: variable '%s' defined but never used",
                     sym->name);
        }
    }

    Scope *parent = currentScope->parent;
    free(currentScope);
    currentScope = parent;
}

/*
 * findSymbol — searches for a symbol by name across all scopes (from the current scope to the global scope).
 * Returns: a pointer to the Symbol, or NULL if not found.
 * The search proceeds from the bottom up: first the current scope, then the parent, and so on.
 * This allows local variables to "shadow" global ones.
*/
static Symbol *findSymbol(const char *name) {
    Scope *scope = currentScope;
    while (scope) {
        for (int i = 0; i < scope->count; i++) {
            if (strcmp(scope->symbols[i].name, name) == 0) {
                return &scope->symbols[i];
            }
        }
        scope = scope->parent;
    }
    return NULL;
}

/* 
 * findSymbolInModule — searches for a function within a specific module.
 * Used for Python and Lua calls of the form lib.func().
 * Parameters:
 *   module — module name (e.g., "lib", "math_utils")
 *   name   — function name (e.g., "greet", "add")
*/
static Symbol *findSymbolInModule(const char *module, const char *name) {
    Scope *scope = currentScope;
    while (scope) {
        for (int i = 0; i < scope->count; i++) {
            Symbol *sym = &scope->symbols[i];
            if (strcmp(sym->name, name) == 0 &&
                strcmp(sym->module, module) == 0) {
                return sym;
            }
        }
        scope = scope->parent;
    }
    return NULL;
}

/* 
 * addSymbol — adds a new symbol to the current scope.
 *
 * Returns: a pointer to the new symbol, or NULL if the scope is full.
 * The symbol is zero-initialized via calloc in pushScope.
*/
static Symbol *addSymbol(const char *name, DataType type,
                         SymbolOrigin origin, int line) {
    if (!currentScope) return NULL;
    if (currentScope->count >= MAX_SYMBOLS_PER_SCOPE) return NULL;

    Symbol *sym = &currentScope->symbols[currentScope->count++];
    strncpy(sym->name, name, sizeof(sym->name) - 1);
    sym->type   = type;
    sym->origin = origin;
    sym->line   = line;
    sym->used   = false;
    return sym;
}

/* 
 * guessReturnTypeFromLine — determines the return type from a prototype line.
 * Examples:
 *   "extern double sin (double __x)" → TYPE_FLOAT
 *   "extern char *strcpy (char *...)" → TYPE_STRING
 *   "extern int printf (const char *...)" → TYPE_INT
 *   "extern void free (void *...)" → TYPE_VOID
*/
static DataType guessReturnTypeFromLine(const char *line) {
    //check char* before char to avoid confusion with 
    if (strstr(line, "char *") || strstr(line, "char*"))  return TYPE_STRING;
    if (strstr(line, "double") || strstr(line, "float"))  return TYPE_FLOAT;
    if (strstr(line, "_Bool")  || strstr(line, "bool"))   return TYPE_BOOL;
    //void* is not void — it is a pointer; we return an int (address)
    if (strstr(line, "void *") || strstr(line, "void*"))  return TYPE_INT;
    if (strstr(line, "void"))                              return TYPE_VOID;
    return TYPE_INT; // DEFULAT 
}

/* 
 * countParamsInProto — counts the number of parameters between ( and ).
 * Returns:
 *   0  — if there are no parameters (void or ())
 *  -1  — if variadic (contains ...)
 *   N  — the number of parameters
*/
static int countParamsInProto(const char *open, const char *close) {
    if (!open || !close || close <= open + 1) return 0;

    char buf[256] = {0};
    strncpy(buf, open + 1, close - open - 1);

    //empty parentheses or just void 
    if (buf[0] == '\0') return 0;
    char *p = buf;
    while (*p == ' ') p++;
    if (strncmp(p, "void", 4) == 0 && !strchr(p, ',')) return 0;

    //Variadic func 
    if (strstr(buf, "...")) return -1;

    //Counting commas — each comma = +1 parameter 
    int count = 1;
    for (char *c = buf; *c; c++) {
        if (*c == ',') count++;
    }
    return count;
}

/*
 * registerFromCHeader — parses a .h file and registers all found functions.
 *
 * Parsing strategy:
 *   1. Read line by line
 *   2. Skip comments, directives (#), and empty lines
 *   3. Look for lines containing '(' and ')' — indicators of a prototype
 *   4. Extract the function name (the word preceding the opening parenthesis)
 *   5. Determine the return type
 *   6. Register as a symbol with ORIGIN_C
*/
static void registerFromCHeader(const char *headerPath) {
    FILE *f = fopen(headerPath, "r");
    if (!f) return;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *p = line;

        // skip space in start
        while (*p == ' ' || *p == '\t') p++;

        // skip comments, dirs, empty line
        if (*p == '#' || *p == '/' || *p == '*' ||
            *p == '\0' || *p == '\n' || *p == '}' || *p == '{') continue;

        // look for an opening parenthesis — indicates a functio
        char *parenOpen = strchr(p, '(');
        if (!parenOpen) continue;

        char *parenClose = strchr(parenOpen, ')');
        if (!parenClose) continue;

        // find the function name: the word immediately before '('
        char *nameEnd = parenOpen - 1;
        // skip spaces before the parenthesis
        while (nameEnd > p && (*nameEnd == ' ' || *nameEnd == '\t')) nameEnd--;

        // move backwards as long as there are alphanumeric characters or _
        char *nameStart = nameEnd;
        while (nameStart > p &&
               (isalnum((unsigned char)*(nameStart - 1)) ||
                *(nameStart - 1) == '_')) {
            nameStart--;
        }

        int nameLen = (int)(nameEnd - nameStart + 1);
        if (nameLen <= 0 || nameLen >= 64) continue;

        char funcName[64] = {0};
        strncpy(funcName, nameStart, nameLen);

        // skip internal symbols (_xxx) and non-functions
        if (funcName[0] == '_') continue;
        if (!isalpha((unsigned char)funcName[0])) continue;

        // do not register if it already exists
        if (findSymbol(funcName)) continue;

        // REG
        DataType retType = guessReturnTypeFromLine(p);
        int paramCount   = countParamsInProto(parenOpen, parenClose);

        Symbol *sym = addSymbol(funcName, retType, ORIGIN_C, 0);
        if (sym) {
            sym->isFunc     = true;
            sym->paramCount = paramCount;
            sym->used       = true; // C func unused 
        }
    }

    fclose(f);
}

/* 
 * findSystemHeaderPath — finds the full path to a system header.
 *
 * First, we query gcc (most reliable), then search manually.
 *
 * Example: "math" → "/usr/include/math.h"
*/
static const char *findSystemHeaderPath(const char *libname) {
    static char path[256];

    // gcc where find header
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
        "gcc -x c /dev/null -E -include %s.h 2>/dev/null | "
        "grep -m1 '# 1 \".*%s\\.h\"' | sed 's/# 1 \"//;s/\".*//'",
        libname, libname);

    FILE *pipe = popen(cmd, "r");
    if (pipe) {
        memset(path, 0, sizeof(path));
        if (fgets(path, sizeof(path), pipe)) {
            path[strcspn(path, "\n")] = '\0';
        }
        pclose(pipe);
        if (strlen(path) > 0) return path;
    }

    // fallback — standar path
    const char *dirs[] = {
        "/usr/include",
        "/usr/local/include",
        "/usr/include/x86_64-linux-gnu",
        "/usr/include/aarch64-linux-gnu",
        NULL
    };

    for (int i = 0; dirs[i]; i++) {
        snprintf(path, sizeof(path), "%s/%s.h", dirs[i], libname);
        FILE *f = fopen(path, "r");
        if (f) { fclose(f); return path; }
    }

    return NULL;
}


/* pyTypeToDataType — convert Python type in DataType */
static DataType pyTypeToDataType(const char *hint) {
    if (!hint || !*hint) return TYPE_INT;
    // skip space in start
    while (*hint == ' ') hint++;

    if (strncmp(hint, "int",   3) == 0) return TYPE_INT;
    if (strncmp(hint, "float", 5) == 0) return TYPE_FLOAT;
    if (strncmp(hint, "str",   3) == 0) return TYPE_STRING;
    if (strncmp(hint, "bool",  4) == 0) return TYPE_BOOL;
    if (strncmp(hint, "None",  4) == 0) return TYPE_VOID;
    return TYPE_INT; // DEFUALT
}

/* 
 * parseReturnTypeFromPyLine — extracts the return type from a def line.
 *
 * Looks for "-> type" before the colon.
 * Example: "def add(a: int, b: int) -> int:" → TYPE_INT
*/
static DataType parseReturnTypeFromPyLine(const char *line) {
    const char *arrow = strstr(line, "->");
    if (!arrow) return TYPE_INT; // DEFUALT

    const char *typeStart = arrow + 2;
    while (*typeStart == ' ') typeStart++;

    // extract the type (up to ':' or a space)
    char typeBuf[32] = {0};
    int i = 0;
    while (*typeStart && *typeStart != ':' &&
           *typeStart != ' ' && i < 31) {
        typeBuf[i++] = *typeStart++;
    }

    return pyTypeToDataType(typeBuf);
}

/* 
 * parseParamTypeFromPyParam — extracts the parameter type.
 *
 * Example: "name: str" → TYPE_STRING
 *          "x"         → TYPE_INT (without annotation)
*/
static DataType parseParamTypeFromPyParam(const char *param) {
    const char *colon = strchr(param, ':');
    if (!colon) return TYPE_INT;

    const char *typeStart = colon + 1;
    while (*typeStart == ' ') typeStart++;

    char typeBuf[32] = {0};
    int i = 0;
    while (*typeStart && *typeStart != ',' &&
           *typeStart != ')' && *typeStart != ' ' && i < 31) {
        typeBuf[i++] = *typeStart++;
    }

    return pyTypeToDataType(typeBuf);
}

/* 
 * registerFromPyFile — parses a .py file and registers all defined functions.
 *
 * Parameters:
 *   path       — path to the .py file
 *   moduleName — module name (e.g., "lib" for "lib.py")
 *                Functions will be accessible as moduleName.funcName()
*/
static void registerFromPyFile(const char *path, const char *moduleName) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Semantic: cannot open Python file: %s\n", path);
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        // skip whitespace
        while (*p == ' ' || *p == '\t') p++;

        // find "def " 
        if (strncmp(p, "def ", 4) != 0) continue;
        p += 4; // skip "def " 

        // extract the function name
        char funcName[64] = {0};
        int i = 0;
        while (*p && *p != '(' && i < 63) {
            funcName[i++] = *p++;
        }
        // remove trailing spaces from the name
        while (i > 0 && funcName[i-1] == ' ') funcName[--i] = '\0';

        if (!*p || *p != '(') continue; //closing '(' not found
        p++; // skip'('

        // gathering parameters 
        char paramsBuf[256] = {0};
        int pi = 0;
        while (*p && *p != ')' && pi < 255) {
            paramsBuf[pi++] = *p++;
        }

        // pars params
        DataType paramTypes[MAX_FUNC_PARAMS] = {0};
        int paramCount = 0;

        char *param = strtok(paramsBuf, ",");
        while (param && paramCount < MAX_FUNC_PARAMS) {
            //skip space
            while (*param == ' ') param++;
            // skip self
            if (strncmp(param, "self", 4) != 0) {
                paramTypes[paramCount++] = parseParamTypeFromPyParam(param);
            }
            param = strtok(NULL, ",");
        }

        // type return
        DataType retType = parseReturnTypeFromPyLine(line);

        // register the function with the module name
        Symbol *sym = addSymbol(funcName, retType, ORIGIN_PYTHON, 0);
        if (sym) {
            sym->isFunc     = true;
            sym->paramCount = paramCount;
            sym->used       = true;
            strncpy(sym->module, moduleName, sizeof(sym->module) - 1);
            for (int j = 0; j < paramCount; j++) {
                sym->paramTypes[j] = paramTypes[j];
            }
        }
    }

    fclose(f);
}

static void registerFromAwlFile(const char *path);

/* parseAwlFuncSignature — parse line:
 * "func add(int a, int b) -> int {"
 * reg func
 */
static void parseAwlFuncSignature(const char *line) {
    const char *p = line;
    while (*p == ' ' || *p == '\t') p++;

    if (strncmp(p, "func ", 5) != 0) return;
    p += 5;

    //name func
    char funcName[64] = {0};
    int i = 0;
    while (*p && *p != '(' && *p != ' ' && i < 63) {
        funcName[i++] = *p++;
    }
    if (!funcName[0]) return;

    // skip '('
    while (*p && *p != '(') p++;
    if (*p != '(') return;
    p++;

    // parse params "int a, string b"
    DataType paramTypes[MAX_FUNC_PARAMS] = {0};
    int paramCount = 0;

    while (*p && *p != ')' && paramCount < MAX_FUNC_PARAMS) {
        while (*p == ' ') p++;
        if (*p == ')') break;

        // type params
        DataType ptype = TYPE_INT;
        if      (strncmp(p, "int",    3) == 0) { ptype = TYPE_INT;    p += 3; }
        else if (strncmp(p, "float",  5) == 0) { ptype = TYPE_FLOAT;  p += 5; }
        else if (strncmp(p, "string", 6) == 0) { ptype = TYPE_STRING; p += 6; }
        else if (strncmp(p, "bool",   4) == 0) { ptype = TYPE_BOOL;   p += 4; }

        paramTypes[paramCount++] = ptype;

        // skipping to the next parameter
        while (*p && *p != ',' && *p != ')') p++;
        if (*p == ',') p++;
    }

    // return type: to be determined -> 
    DataType retType = TYPE_VOID;
    const char *arrow = strstr(line, "->");
    if (arrow) {
        const char *t = arrow + 2;
        while (*t == ' ') t++;
        if      (strncmp(t, "int",    3) == 0) retType = TYPE_INT;
        else if (strncmp(t, "float",  5) == 0) retType = TYPE_FLOAT;
        else if (strncmp(t, "string", 6) == 0) retType = TYPE_STRING;
        else if (strncmp(t, "bool",   4) == 0) retType = TYPE_BOOL;
        else if (strncmp(t, "void",   4) == 0) retType = TYPE_VOID;
    }

    // register 
    Symbol *sym = addSymbol(funcName, retType, ORIGIN_AWL, 0);
    if (sym) {
        sym->isFunc     = true;
        sym->paramCount = paramCount;
        sym->used       = true;
        for (i = 0; i < paramCount; i++) {
            sym->paramTypes[i] = paramTypes[i];
        }
    }
}

static void registerFromAwlFile(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Semantic: cannot open AWL file: %s\n", path);
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (strncmp(p, "func ", 5) == 0) {
            parseAwlFuncSignature(p);
        }
    }

    fclose(f);
}


/* getModuleName — extracts the module name from the file path.
 * example: "libs/hello.py" → "hello"
 */
static void getModuleName(const char *path, char *out, size_t outSize) {
    // only file name, without path dir
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;

    strncpy(out, base, outSize - 1);
    out[outSize - 1] = '\0';

    //removing the extension 
    char *dot = strrchr(out, '.');
    if (dot) *dot = '\0';
}

static void processImport(const char *importPath) {
    size_t len = strlen(importPath);
    char moduleName[64] = {0};
    getModuleName(importPath, moduleName, sizeof(moduleName));

    // Python file
    if (len > 3 && strcmp(importPath + len - 3, ".py") == 0) {
        registerFromPyFile(importPath, moduleName);
        return;
    }

    // AWL libs 
    if (len > 4 && strcmp(importPath + len - 4, ".awl") == 0) {
        registerFromAwlFile(importPath);
        return;
    }

    // C libs: import "mylib.h" 
    if (len > 2 && strcmp(importPath + len - 2, ".h") == 0) {
        hasCImport = true;
        registerFromCHeader(importPath);
        return;
    }

    // system libs C: import "math" 
    hasCImport = true;
    const char *headerPath = findSystemHeaderPath(importPath);
    if (headerPath) {
        registerFromCHeader(headerPath);
    } else {
        fprintf(stderr,
                "Semantic: header '%s.h' not found, "
                "C function calls will not be validated\n",
                importPath);
    }
}

// getTypeName — returns the string representation of the type
static const char *getTypeName(DataType t) {
    switch (t) {
        case TYPE_INT:    return "int";
        case TYPE_FLOAT:  return "float";
        case TYPE_STRING: return "string";
        case TYPE_BOOL:   return "bool";
        case TYPE_VOID:   return "void";
        default:          return "unknown";
    }
}

static DataType getExprType(Node *n) {
    if (!n) return TYPE_INT;

    switch (n->type) {
        case NODE_NUMBER:  return TYPE_INT;
        case NODE_FLOAT:   return TYPE_FLOAT;
        case NODE_STRING:  return TYPE_STRING;
        case NODE_BOOL:    return TYPE_BOOL;

        case NODE_TEXT: {
            Symbol *sym = findSymbol(n->text);
            if (sym && !sym->isFunc) {
                sym->used = true;
                return sym->type;
            }
            return TYPE_INT;
        }

        case NODE_BINOP: {
            /* with one operand float — result float */
            DataType l = getExprType(n->left);
            DataType r = getExprType(n->right);
            if (l == TYPE_FLOAT || r == TYPE_FLOAT) return TYPE_FLOAT;
            return TYPE_INT;
        }

        case NODE_COMPARE:
            // comparison always comes back around bool 
            return TYPE_BOOL;

        case NODE_CALL: {
            // AWL or C func
            Symbol *sym = findSymbol(n->text);
            if (sym && sym->isFunc) return sym->type;
            return TYPE_INT;
        }

        case NODE_PY_CALL: {
            //py func: lib.func()
            Symbol *sym = findSymbolInModule(n->module, n->text);
            if (sym) return sym->type;
            return TYPE_INT;
        }

        default:
            return TYPE_INT;
    }
}

// int == float, translation
static bool typesCompatible(DataType expected, DataType actual) {
    if (expected == actual) return true;
    // int you can pass it where a float is expected.
    if (expected == TYPE_FLOAT && actual == TYPE_INT) return true;
    return false;
}

static void analyzeExpr(Node *n, int line);

static void analyzeExpr(Node *n, int line) {
    if (!n) return;

    switch (n->type) {
        case NODE_NUMBER:
        case NODE_FLOAT:
        case NODE_STRING:
        case NODE_BOOL:
            //literals — nothing to check
            break;

        case NODE_TEXT: {
            // variable — checking if it is defined
            Symbol *sym = findSymbol(n->text);
            if (!sym) addError(SEM_ERR_UNDEFINED_VAR, line, true, "Error: undefined variable '%s'", n->text);
            else sym->used = true; //mark as used

            //var declaration
            if (!n->isDeclaration) n->isDeclaration = true;
            else n->isDeclaration = false;
            break;
        }

        case NODE_BINOP: {
            // division 0
            if (n->op == TOK_SLASH && n->right &&
                n->right->type == NODE_NUMBER && n->right->number == 0) {
                addError(SEM_ERR_DIVIDE_BY_ZERO, line, true,
                         "Error: division by zero");
            }
            analyzeExpr(n->left,  line);
            analyzeExpr(n->right, line);
            break;
        }

        case NODE_COMPARE:
            analyzeExpr(n->left,  line);
            analyzeExpr(n->right, line);
            break;

        case NODE_CALL: {
            Symbol *sym = findSymbol(n->text);
            if (!sym) {
                if (!hasCImport) {
                    // no C imports — function is not strictly defined
                    addError(SEM_ERR_UNDEFINED_FUNC, line, true,
                             "Error: undefined function '%s'", n->text);
                }
                // there are C imports — do not report an error; it might be from a .h file
            } else if (sym->isFunc && sym->paramCount >= 0) {
                // we know the exact number of parameters — we verify 
                if (n->argCount != sym->paramCount) {
                    addError(SEM_ERR_WRONG_ARG_COUNT, line, true,
                             "Error: function '%s' expects %d argument(s) but got %d",
                             n->text, sym->paramCount, n->argCount);
                } else {
                    // check argument types (only for AWL functions)
                    if (sym->origin == ORIGIN_AWL) {
                        for (int i = 0; i < n->argCount; i++) {
                            DataType argType  = getExprType(n->args[i]);
                            DataType expected = sym->paramTypes[i];
                            if (!typesCompatible(expected, argType)) {
                                addError(SEM_ERR_TYPE_MISMATCH, line, false,
                                         "Warning: argument %d of '%s': "
                                         "expected %s, got %s",
                                         i + 1, n->text,
                                         getTypeName(expected),
                                         getTypeName(argType));
                            }
                        }
                    }
                }
            } else if (sym->isFunc && sym->paramCount == -1) {
                // paramCount == -1: variadic - we do not check
            }

            // recursively analyze the arguments
            for (int i = 0; i < n->argCount; i++) {
                analyzeExpr(n->args[i], line);
            }
            break;
        }

        case NODE_PY_CALL: {
            Symbol *sym = findSymbolInModule(n->module, n->text);

            if (!sym) {
                addError(SEM_ERR_UNDEFINED_FUNC, line, true,
                         "Error: undefined function '%s.%s' "
                         "(check that '%s.py' is imported and contains this function)",
                         n->module, n->text, n->module);
            } else if (sym->paramCount >= 0 &&
                       n->argCount != sym->paramCount) {
                addError(SEM_ERR_WRONG_ARG_COUNT, line, true,
                         "Error: Python function '%s.%s' expects %d argument(s) "
                         "but got %d",
                         n->module, n->text,
                         sym->paramCount, n->argCount);
            }

            for (int i = 0; i < n->argCount; i++) {
                analyzeExpr(n->args[i], line);
            }
            break;
        }

        case NODE_INDEX: {
            // index array: arr[i] 
            Symbol *sym = findSymbol(n->text);
            if (!sym) {
                addError(SEM_ERR_UNDEFINED_VAR, line, true,
                         "Error: undefined variable '%s'", n->text);
            } else {
                sym->used = true;
            }
            analyzeExpr(n->left, line); // index
            break;
        }

        default:
            break;
    }
}

static void analyzeStmt(Node *n);

static void analyzeBlock(Node *block) {
    if (!block) return;
    pushScope();
    for (int i = 0; i < block->stmtCount; i++) {
        analyzeStmt(block->statements[i]);
    }
    popScope();
}

/*
 * blockHasReturn — checks whether a return statement is guaranteed to execute.
 *
 * A "guaranteed" return is:
 *   1. A return directly within the block
 *   2. An if-else statement where BOTH branches contain a return
 *      (an if without an else does not count as guaranteed)
 *
 * A while loop does NOT count — it might not execute.
*/
static bool blockHasReturn(Node *block) {
    if (!block) return false;

    for (int i = 0; i < block->stmtCount; i++) {
        Node *stmt = block->statements[i];
        if (!stmt) continue;

        // return
        if (stmt->type == NODE_RETURN) return true;

        // if-else: has return
        if (stmt->type == NODE_IF &&
            stmt->thenBranch && stmt->elseBranch &&
            blockHasReturn(stmt->thenBranch) &&
            blockHasReturn(stmt->elseBranch)) {
            return true;
        }
    }
    return false;
}

static void analyzeStmt(Node *n) {
    if (!n) return;

    switch (n->type) {

        case NODE_IMPORT:
            processImport(n->text);
            break;

        case NODE_ASSIGN: {
            analyzeExpr(n->left, n->number); 

            DataType exprType = getExprType(n->left);
            Symbol *existing = findSymbol(n->text);

            if (!existing) {
                // new var, reg
                Symbol *sym = addSymbol(n->text, exprType, ORIGIN_AWL, 0);
                if (sym) sym->used = false; //not used
            } else {
                // reassignment — checking type compatibility
                existing->used = true;
                if (!typesCompatible(existing->type, exprType)) {
                    addError(SEM_ERR_TYPE_MISMATCH, 0, false,
                             "Warning: assigning %s to variable '%s' of type %s",
                             getTypeName(exprType),
                             n->text,
                             getTypeName(existing->type));
                }
            }
            break;
        }

        case NODE_PRINT:
            // print expr; — check expr
            analyzeExpr(n->expr, 0);
            break;

        case NODE_RETURN: {
            if (currentFuncReturnType == TYPE_VOID && n->expr) {
                addError(SEM_ERR_RETURN_IN_VOID, 0, true,
                         "Error: function '%s' is void but returns a value",
                         currentFuncName);
                break;
            }

            if (n->expr) {
                analyzeExpr(n->expr, 0);
                DataType retType = getExprType(n->expr);
                if (currentFuncReturnType != TYPE_VOID &&
                    !typesCompatible(currentFuncReturnType, retType)) {
                    addError(SEM_ERR_WRONG_RETURN_TYPE, 0, true,
                             "Error: function '%s' should return %s but returns %s",
                             currentFuncName,
                             getTypeName(currentFuncReturnType),
                             getTypeName(retType));
                }
            }
            break;
        }

        case NODE_IF:{
            analyzeExpr(n->cond, 0);
            analyzeBlock(n->thenBranch);
            if (n->elseBranch) analyzeBlock(n->elseBranch);
            break;
        }

        case NODE_WHILE: {
            analyzeExpr(n->cond, 0);
            analyzeBlock(n->thenBranch);
            break;
        }

        case NODE_FOR: {
            analyzeExpr(n->left, 0); // range cycle

            pushScope();
            // 'i' for cycle 'for'
            Symbol *iSym = addSymbol("i", TYPE_INT, ORIGIN_AWL, 0);
            if (iSym) iSym->used = true; 

            if (n->thenBranch) {
                for (int i = 0; i < n->thenBranch->stmtCount; i++) {
                    analyzeStmt(n->thenBranch->statements[i]);
                }
            }
            popScope();
            break;
        }

        case NODE_BLOCK:
            analyzeBlock(n);
            break;

        case NODE_EXPR_STMT:
            // expr how statement: foo();
            analyzeExpr(n->expr, 0);
            break;

        case NODE_INDEX_ASSIGN: {
            Symbol *sym = findSymbol(n->text);
            if (!sym) {
                addError(SEM_ERR_UNDEFINED_VAR, 0, true,
                         "Error: undefined variable '%s'", n->text);
            } else {
                sym->used = true;
            }
            analyzeExpr(n->left, 0);  // index
            analyzeExpr(n->right, 0); // new value
            break;
        }

        case NODE_FUNC_DECL:
            // skip — functions are analyzed separately in analyzeAST
            break;

        default:
            break;
    }
}


static void analyzeFunc(Node *n) {
    if (!n) return;

    // VAR FOR ANALYZE
    DataType savedReturnType = currentFuncReturnType;
    char savedFuncName[64];
    strncpy(savedFuncName, currentFuncName, sizeof(savedFuncName) - 1);

    // CONTEXT FUNC
    currentFuncReturnType = n->returnType;
    strncpy(currentFuncName, n->text, sizeof(currentFuncName) - 1);

    pushScope();

    // reg params how local var
    for (int i = 0; i < n->paramCount; i++) {
        Symbol *p = addSymbol(n->params[i].name,
                              n->params[i].type,
                              ORIGIN_AWL, 0);
        if (p) {
            p->used = true; // params used
        }
    }

    // analyze block
    if (n->thenBranch) {
        for (int i = 0; i < n->thenBranch->stmtCount; i++) {
            analyzeStmt(n->thenBranch->statements[i]);
        }
    }

    // check return func(if isnt void)
    if (n->returnType != TYPE_VOID && !blockHasReturn(n->thenBranch)) {
        addError(SEM_ERR_MISSING_RETURN, 0, true,
                 "Error: function '%s' must return %s but has no guaranteed return statement",
                 n->text, getTypeName(n->returnType));
    }

    popScope();

    // restoring context
    currentFuncReturnType = savedReturnType;
    strncpy(currentFuncName, savedFuncName, sizeof(currentFuncName) - 1);
}

SemResult analyzeAST(Node *ast) {

    //VAR ANALYZE 
    memset(&result, 0, sizeof(SemResult));
    currentScope          = NULL;
    currentFuncReturnType = TYPE_VOID;
    currentFuncName[0]    = '\0';

    if (!ast) return result;

    // global scope
    pushScope();

    //IMPORT BLOCK
    for (int i = 0; i < ast->stmtCount; i++) {
        Node *stmt = ast->statements[i];
        if (stmt && stmt->type == NODE_IMPORT) {
            processImport(stmt->text);
        }
    }

    //FUNC BLOCK
    for (int i = 0; i < ast->stmtCount; i++) {
        Node *stmt = ast->statements[i];
        if (!stmt || stmt->type != NODE_FUNC_DECL) continue;

        Symbol *sym = addSymbol(stmt->text, stmt->returnType, ORIGIN_AWL, 0);
        if (sym) {
            sym->isFunc     = true;
            sym->paramCount = stmt->paramCount;
            sym->used       = true;
            for (int j = 0; j < stmt->paramCount; j++) {
                sym->paramTypes[j] = stmt->params[j].type;
            }
        }
    }

    // ANALYZE STMT
    for (int i = 0; i < ast->stmtCount; i++) {
        Node *stmt = ast->statements[i];
        if (!stmt) continue;

        if (stmt->type == NODE_FUNC_DECL) {
            analyzeFunc(stmt);
        } else if (stmt->type != NODE_IMPORT) {
            analyzeStmt(stmt);
        }
    }

    popScope(); //close global scope
    return result;
}

/*
 * printSemErrors — prints errors and warnings to the terminal.
 * Errors are displayed in red, warnings in yellow (if the terminal supports it).
 */
void printSemErrors(SemResult *result) {
    if (!result || result->count == 0) return;

    int errorCount   = 0;
    int warningCount = 0;

    for (int i = 0; i < result->count; i++) {
        SemError *err = &result->errors[i];

        if (err->isFatal) {
            // red color for error
            printf("\033[31m[ERROR]\033[0m");
            if (err->line > 0) printf(" (line %d)", err->line);
            printf(" %s\n", err->message);
            errorCount++;
        } else {
            // yellow color for warn
            printf("\033[33m[WARNING]\033[0m");
            if (err->line > 0) printf(" (line %d)", err->line);
            printf(" %s\n", err->message);
            warningCount++;
        }
    }

    printf("\n");
    if (errorCount > 0 || warningCount > 0) {
        printf("Result: %d error(s), %d warning(s)\n",
               errorCount, warningCount);
    }
    if (result->hasErrors) {
        printf("Compilation aborted due to errors.\n");
    }
}