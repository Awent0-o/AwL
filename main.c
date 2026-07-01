#include "lexer/lexer.h"
#include "codegen/codegen.h"
#include "parser/parser.h"

const char *get_file_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

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
        fprintf(stderr, "how to use %s: <file.awl>\n", argv[0]);
        return 1;
    }
    if (strcmp(get_file_ext(argv[1]), "awl") != 0){
        fprintf(stderr, "you must use %s: <file.awl>\n", argv[0]);
        return 2;
    }

    char out_filename[512] = "a"; 

    if (argc > 3 && strcmp(argv[2], "-o") == 0) {
        strncpy(out_filename, argv[3], sizeof(out_filename) - 5);
        out_filename[sizeof(out_filename) - 5] = '\0'; 
    }

    char *dot = strchr(out_filename, '.');
    if (dot != NULL) {
        *dot = '\0'; 
    }

    size_t len;
    char *src = readFile(argv[1], &len);
    printf("1. File read: %zu bytes\n", len);

    TokenList tokens = tokenize(src, len);
    printf("2. Tokenized: %d tokens\n", tokens.count);

    Parser parser = { .tokens = &tokens, .pos = 0 };
    printf("3. Starting parse...\n");

    Node *ast = parseProgram(&parser);
    printf("4. Parsed OK\n");

    char c_filename[512];
    sprintf(c_filename, "%s.c", out_filename);

    FILE *out = fopen(c_filename, "w");
    printf("5. Starting codegen...\n");

    if (!out) {
        fprintf(stderr, "Error opening output file %s\n", c_filename);
        return 3;
    }
    bool compileToBin = (argc >= 3 && strcmp(argv[2], "-c") == 0);

    genC(ast, out);
    fclose(out);

    if (compileToBin) {
        genBin(out_filename);
    }

    printf("gen %s\n", out_filename);

    free(src);
    free(tokens.tokens);
    return 0;
}