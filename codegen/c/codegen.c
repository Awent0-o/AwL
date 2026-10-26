#include "codegen_internal.h"

bool hasPython = false;

//PY VAR
char pyFiles[16][256];
int pyFileCount = 0;

void genC(Node *ast, FILE *out) {
    if (!ast) {
        printf("genC: NULL ast!\n");
        return;
    }
    declaredCount = 0; // reset declared vars for each file

    //IMPORT ALL
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <stdbool.h>\n");
    fprintf(out, "#include <string.h>\n");
    for (int i = 0; i < ast->stmtCount; i++) {
        if (ast->statements[i]->type == NODE_IMPORT) {
            genImport(ast->statements[i], out);
        }
    }

    //HELPER
    fprintf(out, "%s\n", AUTOPRINTTYPE);// AUTO TYPE PRINT
    //ARAYS TYPE IF YOU CREATE NEW ARR IN FUNC
    fprintf(out, "#define AWL_ARRAY_RETURN_SIZE 256\n");
    fprintf(out, "static int _awl_ret_buf[AWL_ARRAY_RETURN_SIZE];\n\n");

    Node *entry = NULL;

    // pre-register function signatures so calls know return/pointer types
    for (int i = 0; i < ast->stmtCount; i++) {
        Node *stmt = ast->statements[i];
        if (isMain(stmt)) {
            entry = stmt;
        } else if (stmt->type == NODE_FUNC_DECL) {
            regVar(stmt->text, stmt->returnType, stmt->returnElementType);
        }
    }

    //FUNC DECL (user main is emitted last as the real entry point)
    for (int i = 0; i < ast->stmtCount; i++) {
        Node *stmt = ast->statements[i];
        if (stmt->type == NODE_FUNC_DECL && !isMain(stmt)) {
            genFuncDecl(stmt, out);
        }
    }

    //MAIN FUNC — auto-generated or based on user func main()
    genMainEntry(ast, entry, out);
}

void genBin(const char *filename) {
    char compile_cmd[2048];
    char flagsComp[1024] = "";
    char remove_cmd[256];
    int res = 0;

    snprintf(compile_cmd, sizeof(compile_cmd), "gcc -w %s.c", filename);
    // detect gcc flag
    if (flags.debugInfo)     strncat(flagsComp, " -g",
                                     sizeof(flagsComp) - strlen(flagsComp) - 1);
    if (flags.optimize1)     strncat(flagsComp, " -O1",
                                     sizeof(flagsComp) - strlen(flagsComp) - 1);
    if (flags.optimize2)     strncat(flagsComp, " -O2",
                                     sizeof(flagsComp) - strlen(flagsComp) - 1);
    if (flags.optimize3)     strncat(flagsComp, " -O3",
                                     sizeof(flagsComp) - strlen(flagsComp) - 1);
    if (flags.wallWarnings)  strncat(flagsComp, " -Wall",
                                     sizeof(flagsComp) - strlen(flagsComp) - 1);
    if (flags.wextra)        strncat(flagsComp, " -Wextra",
                                     sizeof(flagsComp) - strlen(flagsComp) - 1);

    // Python flag
    if (hasPython) {
        strncat(flagsComp, " $(python3-config --includes --ldflags --embed)",
                sizeof(flagsComp) - strlen(flagsComp) - 1);
    }

    if (flags.emitAsm) {
        //gen .s file
        snprintf(compile_cmd, sizeof(compile_cmd), "gcc %s.c%s -S -o %s.s",
                 filename, flagsComp, filename);
        if (flags.verbose) printf("Assembling: %s\n", compile_cmd);
        system(compile_cmd);

        // compile .s file
        snprintf(compile_cmd, sizeof(compile_cmd), "gcc %s.s%s -o %s",
                 filename, flagsComp, filename);
    }
    else {
        snprintf(compile_cmd, sizeof(compile_cmd), "gcc %s.c%s -o %s",
                 filename, flagsComp, filename);
    }

    if (flags.verbose) printf("Compiling: %s\n", compile_cmd);

    //out file
    char out[256];
    snprintf(out, sizeof(out), " -o %s", filename);

    // full command
    strncat(remove_cmd, flagsComp, sizeof(compile_cmd) - strlen(compile_cmd) - 1);
    strncat(compile_cmd, out, sizeof(compile_cmd) - strlen(compile_cmd) - 1);

    res = system(compile_cmd);
    if (res != 0) {
        fprintf(stderr, "Compilation failed!\n");
    }

    snprintf(remove_cmd, sizeof(remove_cmd), "rm %s.c", filename);

    if (flags.verbose) printf("Binary: %s\n", filename);

    //delete .c if keepc = false
    if (!flags.keepC) {
        char rm[520];
        snprintf(rm, sizeof(rm), "rm %s.c", filename);
        system(rm);
    }
    else {
        fprintf(stderr, "Compilation failed! Temporary .c file was preserved.\n");
    }
}