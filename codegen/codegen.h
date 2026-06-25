#include "../lexer/ast.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>


void genC(Node *ast, FILE *out);

void genBin(const char* filename);