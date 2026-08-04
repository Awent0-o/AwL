#ifndef CODEGEN_H
#define CODEGEN_H

#include "lexer/lexer.h"
#include "lexer/ast.h"
#include "parser/parser.h"
#include "codegen/python/codepy.h"
#include "utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define AUTOPRINTTYPE \
"#define print(x) _Generic((x), \\\n" \
"    float: printf(\"%f\", x), \\\n" \
"    char*: printf(\"%s\", x), \\\n" \
"    char: printf(\"%c\", x), \\\n" \
"    int: printf(\"%d\", x), \\\n" \
"    bool: printf(\"%d\", x), \\\n" \
"    default: printf(\"undefine type\") \\\n" \
")\n\n"

void genStmt(Node *stmt, FILE *out);

void genC(Node *ast, FILE *out);

void genBin(const char* filename);

#endif