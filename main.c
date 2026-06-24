#include <stdio.h>
#include <stdlib.h>

#include "lexer/lexer.h"
#include "parser/parser.h"

char *readFile(const char *path, size_t *outLen) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "not such file: %s\n", path);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = malloc(size + 1);
    fread(buffer, 1, size, f);
    buffer[size] = '\0';

    fclose(f);
    *outLen = (size_t)size;
    return buffer;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "use: %s <file.awl>\n", argv[0]);
        return 1;
    }

    size_t len;
    char *src = readFile(argv[1], &len);

    TokenList tokens = tokenize(src, len);

    Parser parser = { .tokens = &tokens, .pos = 0 };
    Node *ast = parseProgram(&parser);

    // TODO interpreter code
    printf("succes.\n");

    free(src);
    free(tokens.tokens);
    return 0;
}