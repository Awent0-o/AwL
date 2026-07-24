#include "codegen/c/codegen.h"
#include "semantic/semantic.h"

int main(int argc, char *argv[]) {
    initFlags();
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


    size_t len;
    char *src = readFile(argv[1], &len);

    TokenList tokens = tokenize(src, len);

    Parser parser = { .tokens = &tokens, .pos = 0 };

    Node *ast = parseProgram(&parser);

    SemResult result = analyzeAST(ast);
    printSemErrors(&result);
    if(result.hasErrors) return -1;
    

    char c_filename[520];
    snprintf(c_filename, sizeof(c_filename), "%s.c", out_filename);

    FILE *out = fopen(c_filename, "w");

    if (!out) {
        fprintf(stderr, "Error opening output file %s\n", c_filename);
        return 3;
    }

    bool compileToC = (argc >= 3 && strcmp(argv[2], "-c") == 0);

    if(compileThis){
        genC(ast, out);
        fclose(out);
    
        if (!compileToC) {
            genBin(out_filename);
        }
    
        printf("gen %s\n", out_filename);
    }

    freeNode(ast);
    freePyModules();
    free(src);
    free(tokens.tokens);
    return 0;
}