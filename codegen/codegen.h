#include "../lexer/ast.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Визначаємо підтримувані типи даних
typedef enum {
    TYPE_UNKNOWN = -1,
    TYPE_INT = 0,
    TYPE_STRING = 1,
    TYPE_FLOAT = 2,
    TYPE_BOOL = 3,
    TYPE_ARRAY = 4
} DataType;

typedef struct {
    char name[64];
    DataType type;
    DataType elementType;
} Variable;


void genC(Node *ast, FILE *out);

void genBin(const char* filename);