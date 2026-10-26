#ifndef CODEGEN_INTERNAL_H
#define CODEGEN_INTERNAL_H

#include "codegen/c/codegen.h"
#include "codegen/python/codepy.h"
#include "utils/utils.h"

#include <stdbool.h>
#include <stdio.h>

// Shared codegen state
extern bool hasPython;
extern char pyFiles[16][256];
extern int pyFileCount;

// Variable table (utils_codegen.c)
extern int declaredCount;

DataType getVarType(const char *name);
DataType getVarElementType(const char *name);
Variable *getVar(const char *name);
void regVar(const char *name, DataType type, DataType elementType);
const char *getTypeStr(DataType type);
const char *getFormatSpec(DataType t);
DataType ExprType(Node *n);
DataType ExprElementType(Node *n);

// Main helpers (utils_codegen.c)
bool isMain(Node *n);
void genMainParams(Node *entry, FILE *out);

// Expressions (expr_codegen.c)
void genExpr(Node *n, FILE *out);
void genFString(const char *template, FILE *out);

// Statements / declarations (stmt_codegen.c)
void genBlock(Node *n, FILE *out);
void genStmt(Node *n, FILE *out);
void genFuncDecl(Node *n, FILE *out);
void genMainEntry(Node *ast, Node *entry, FILE *out);

// Imports (import_codegen.c)
void genImport(Node *n, FILE *out);

#endif
