#include "codegen/c/codegen.h"
#include "semantic/semantic.h"

int main(int argc, char *argv[]) {
    initFlags();
    parseArgs(argc, argv);

    // -h / --help
    if (flags.showHelp) {
        printHelp(argv[0]);
        return 0;
    }

    // --version 
    if (flags.showVersion) {
        printVersion();
        return 0;
    }

    // verif input file
    if (flags.inputFile[0] == '\0') {
        fprintf(stderr, RED"Error: " WHITE"no input file\n");
        fprintf(stderr, "Run '%s --help' for usage\n", argv[0]);
        return 1;
    }

    // timer
    struct timespec startTime, endTime;
    if (flags.showTime) {
        clock_gettime(CLOCK_MONOTONIC, &startTime);
    }

    // read file
    size_t len;
    char *src = readFile(flags.inputFile, &len);
    if (!src) return 1;

    if (flags.verbose) printf(MAGENTA"Read: %s (%zu bytes)\n", flags.inputFile, len);

    //TOKENIZE
    TokenList tokens = tokenize(src, len);

    //PARSE PROGRAM
    Parser parser = { .tokens = &tokens, .pos = 0 };
    Node *ast = parseProgram(&parser);


    // SEMANTIC ANALYZE
    SemResult result = analyzeAST(ast);
    printSemErrors(&result);
    if(result.hasErrors) return -1;
    

    char c_filename[520];
    snprintf(c_filename, sizeof(c_filename), "%s.c", flags.outputName);

    FILE *out = fopen(c_filename, "w");

    if (!out) {
        fprintf(stderr, RED"Error opening output file %s\n", c_filename);
        return 3;
    }
    if(compileThis) return 4;
    genC(ast, out);
    fclose(out);

    // compile to bin
    if (!flags.compileToC) {
        genBin(flags.outputName);
    }
    else{
        if (flags.verbose) printf(MAGENTA"Generated: %s\n", c_filename);
    }

    // time compile
    if (flags.showTime) {
        clock_gettime(CLOCK_MONOTONIC, &endTime);
        double elapsed = (endTime.tv_sec  - startTime.tv_sec) +
                         (endTime.tv_nsec - startTime.tv_nsec) / 1e9;
        printf(CYAN_BRIGHT"Time: %.3f seconds\n", elapsed);
    }

    freeNode(ast);
    freePyModules();
    free(src);
    free(tokens.tokens);
    return 0;
}