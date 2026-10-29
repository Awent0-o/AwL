#include "codegen_internal.h"

// VAR CODEGEN
#define MAX_VARS 256

static Variable declaredVars[MAX_VARS];
int declaredCount = 0;

DataType getVarType(const char *name) {
    for (int i = 0; i < declaredCount; i++) {
        if (strcmp(declaredVars[i].name, name) == 0) return declaredVars[i].type;
    }
    return TYPE_UNKNOWN;
}

DataType getVarElementType(const char *name) {
    for (int i = 0; i < declaredCount; i++) {
        if (strcmp(declaredVars[i].name, name) == 0) {
            if (declaredVars[i].type == TYPE_ARRAY ||
                declaredVars[i].type == TYPE_POINTER) {
                return declaredVars[i].elementType;
            }
            return declaredVars[i].type;
        }
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
    //check if already declared
    for (int i = 0; i < declaredCount; i++) {
        if (strcmp(declaredVars[i].name, name) == 0) {
            //update type if already declared
            declaredVars[i].type = type;
            declaredVars[i].elementType = elementType;
            return;
        }
    }
    declaredVars[declaredCount].name = _strdup(name);
    declaredVars[declaredCount].type = type;
    declaredVars[declaredCount].elementType = elementType;
    declaredCount++;
}

const char *getTypeStr(DataType type) {
    switch (type) {
        case TYPE_INT: return "int";
        case TYPE_STRING: return "char*";
        case TYPE_FLOAT: return "float";
        case TYPE_BOOL: return "bool";
        case TYPE_VOID: return "void";
        case TYPE_AUTO: return "__auto_type";
        case TYPE_POINTER: return "void*";
        default: return "unknown";
    }
}

// getFormatSpec — return type for printf
const char *getFormatSpec(DataType t) {
    switch (t) {
        case TYPE_FLOAT:  return "%f";
        case TYPE_STRING: return "%s";
        case TYPE_BOOL:   return "%d";
        case TYPE_POINTER: return "%p";
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
        case NODE_ADDR:   return TYPE_POINTER;
        case NODE_NOT:    return TYPE_BOOL;

        case NODE_DEREF: {
            if (n->left && n->left->type == NODE_TEXT) {
                DataType t = getVarType(n->left->text);
                if (t == TYPE_POINTER || t == TYPE_ARRAY) {
                    return getVarElementType(n->left->text);
                }
                return t;
            }
            return TYPE_INT;
        }

        case NODE_TEXT: {
            // var = var2
            DataType t = getVarType(n->text);
            return (t != TYPE_UNKNOWN) ? t : TYPE_AUTO;
        }
        case NODE_BINOP: {
            if (n->op == TOK_AND || n->op == TOK_OR) return TYPE_BOOL;

            // pointer arithmetic keeps the pointer type
            if ((n->op == TOK_PLUS || n->op == TOK_MINUS) &&
                ExprType(n->left) == TYPE_POINTER) {
                return TYPE_POINTER;
            }

            // if one operand float, all do float
            if (ExprType(n->left) == TYPE_FLOAT ||
                ExprType(n->right) == TYPE_FLOAT) {
                return TYPE_FLOAT;
            }
            return TYPE_AUTO;
        }

        case NODE_ARRAY: {
            Variable *var = getVar(n->text);
            if (var) var->elementType = n->returnElementType;
            return TYPE_ARRAY;
        }

        case NODE_CALL: {
            Variable *var = getVar(n->text);
            if (var) {
                //if func return arr - return TYPE_ARRAY
                //elementType we take separately
                return var->type;
            }
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
        case NODE_INDEX: {
            // arr[i] — element type
            Variable *v = getVar(n->text);
            if (v && (v->type == TYPE_ARRAY || v->type == TYPE_POINTER)) {
                return v->elementType;
            }
            return TYPE_INT;
        }

        default: return TYPE_AUTO;
    }
}

// Type of the value a pointer/array/address expression points to.
DataType ExprElementType(Node *n) {
    if (!n) return TYPE_UNKNOWN;

    switch (n->type) {
        case NODE_ADDR:
            if (n->left && n->left->type == NODE_TEXT)
                return getVarElementType(n->left->text);
            return ExprType(n->left);

        case NODE_DEREF:
            if (n->left && n->left->type == NODE_TEXT) {
                DataType t = getVarType(n->left->text);
                if (t == TYPE_POINTER) return getVarElementType(n->left->text);
                return t;
            }
            return TYPE_INT;

        case NODE_TEXT: {
            DataType t = getVarType(n->text);
            if (t == TYPE_POINTER || t == TYPE_ARRAY)
                return getVarElementType(n->text);
            return t;
        }

        case NODE_BINOP:
            if (ExprType(n->left) == TYPE_POINTER)
                return ExprElementType(n->left);
            return ExprElementType(n->right);

        case NODE_CALL:
            return getVarElementType(n->text);

        case NODE_INDEX:
            return ExprType(n);

        default:
            return ExprType(n);
    }
}

bool isMain(Node *n) {
    return n && n->type == NODE_FUNC_DECL &&
           n->text && strcmp(n->text, "main") == 0;
}

void genMainParams(Node *entry, FILE *out) {
    if (!entry || entry->paramCount == 0) {
        fprintf(out, "void");
        return;
    }

    for (int i = 0; i < entry->paramCount; i++) {
        if (i > 0) fprintf(out, ", ");

        Param *param = &entry->params[i];
        if (param->type == TYPE_ARRAY || param->type == TYPE_POINTER) {
            fprintf(out, "%s *%s", getTypeStr(param->elementType), param->name);
        } else {
            fprintf(out, "%s %s", getTypeStr(param->type), param->name);
        }
    }
}

