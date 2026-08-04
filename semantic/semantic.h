#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "lexer/lexer.h"
#include "lexer/ast.h"
#include "utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>


typedef enum {
    // ERROR HANDLES
    SEM_ERR_MISSING_RETURN,     // func add() -> int { } — no return
    SEM_ERR_RETURN_IN_VOID,     // func f() -> void { return 5; } — return in void
    SEM_ERR_WRONG_RETURN_TYPE,  // func f() -> int { return "hi"; } — incorrect type
    SEM_ERR_UNDEFINED_FUNC,     // foo() — function is undefined and not imported
    SEM_ERR_WRONG_ARG_COUNT,    // add(1,2,3) where add takes 2 — incorrect number of arguments

    // VAR ERROR
    SEM_ERR_UNDEFINED_VAR,      // print x — x not undefined
    SEM_ERR_TYPE_MISMATCH,      // x = "hello" where x: int — incompatible types

    // ERROR LOGIC
    SEM_ERR_DIVIDE_BY_ZERO,     // x = 5 / 0 — division zero

    // WARN
    SEM_WARN_UNUSED_VAR,        // x = 5; (x not used)
    SEM_WARN_UNKNOWN_ARG_COUNT, // C func find but not vision arguments
} SemErrorType;

typedef struct {
    SemErrorType type;
    int line;
    char message[512];
    bool isFatal;
} SemError;

#define MAX_SEM_ERRORS 128

typedef struct {
    SemError errors[MAX_SEM_ERRORS];
    int count;
    bool hasErrors;
} SemResult;

typedef enum {
    ORIGIN_AWL,    //defined in .awl file
    ORIGIN_C,      //find in C header (.h file or math)
    ORIGIN_PYTHON, //find in Python file(.py)
} SymbolOrigin;


#define MAX_FUNC_PARAMS 8

typedef struct {
    char name[64];          // name symbol
    char module[64];        // for Python/Lua: name module (lib, math_utils)

    DataType type;          // type (for var) or type return 
    bool isFunc;            
    SymbolOrigin origin;    // where this symbol came from

    //fields for functions
    int paramCount;                      // number of parameters (-1 = variadic/unknown)
    DataType paramTypes[MAX_FUNC_PARAMS]; // type params
    DataType elementType;

    //fields for variables
    bool used;    // whether it is used (for the unused variable warning)
    int line;     // definition line (for error messages)
} Symbol;

#define MAX_SYMBOLS_PER_SCOPE 256

typedef struct Scope {
    Symbol symbols[MAX_SYMBOLS_PER_SCOPE];
    int count;
    struct Scope *parent; 
} Scope;

SemResult analyzeAST(Node *ast);

void printSemErrors(SemResult *result);

#endif