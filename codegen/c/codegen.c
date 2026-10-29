#include "codegen_internal.h"

bool hasPython = false;

//PY VAR
char pyFiles[16][256];
int pyFileCount = 0;

//CPP VAR
char cppFiles[16][256];
int cppFileCount = 0;

void genC(Node *ast, FILE *out) {
    if (!ast) {
        printf("genC: NULL ast!\n");
        return;
    }
    declaredCount = 0; // reset declared vars for each file
    cppFileCount = 0;  // reset C++ imports for each file

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

static void addFlag(char *dst, size_t cap, const char *flag) {
    strncat(dst, flag, cap - strlen(dst) - 1);
}

void genBin(const char *filename) {
    char compile_cmd[8192];
    char flagsComp[1024] = " -I.";
    int res = 0;

    // detect gcc flag
    if (flags.debugInfo)     addFlag(flagsComp, sizeof(flagsComp), " -g");
    if (flags.optimize1)     addFlag(flagsComp, sizeof(flagsComp), " -O1");
    if (flags.optimize2)     addFlag(flagsComp, sizeof(flagsComp), " -O2");
    if (flags.optimize3)     addFlag(flagsComp, sizeof(flagsComp), " -O3");
    if (flags.wallWarnings)  addFlag(flagsComp, sizeof(flagsComp), " -Wall");
    if (flags.wextra)        addFlag(flagsComp, sizeof(flagsComp), " -Wextra");

    // Python flag
    if (hasPython) {
        addFlag(flagsComp, sizeof(flagsComp),
                " $(python3-config --includes --ldflags --embed)");
    }

    char c_obj[1024] = "";
    char cpp_obj[16][1024];
    bool hasCpp = cppFileCount > 0;
    bool ok = true;

    for (int i = 0; i < cppFileCount; i++) {
        snprintf(cpp_obj[i], sizeof(cpp_obj[i]),
                 "%s.awl_cpp%d.o", filename, i);

        snprintf(compile_cmd, sizeof(compile_cmd),
                 "g++ -w -std=c++17 %s%s -c -o %s",
                 cppFiles[i], flagsComp, cpp_obj[i]);

        if (flags.verbose) printf("Compiling C++: %s\n", compile_cmd);
        res = system(compile_cmd);
        if (res != 0) {
            fprintf(stderr, "C++ compilation failed: %s\n", cppFiles[i]);
            ok = false;
            break;
        }
    }

    if (ok && flags.emitAsm) {
        snprintf(compile_cmd, sizeof(compile_cmd),
                 "gcc %s.c%s -S -o %s.s", filename, flagsComp, filename);

        if (flags.verbose) printf("Assembling: %s\n", compile_cmd);
        res = system(compile_cmd);
        if (res != 0) {
            fprintf(stderr, "Compilation failed!\n");
            ok = false;
        }

        if (ok) {
            if (hasCpp) {
                snprintf(compile_cmd, sizeof(compile_cmd),
                         "g++ -w %s.s", filename);
                for (int i = 0; i < cppFileCount; i++) {
                    addFlag(compile_cmd, sizeof(compile_cmd), " ");
                    addFlag(compile_cmd, sizeof(compile_cmd), cpp_obj[i]);
                }
                addFlag(compile_cmd, sizeof(compile_cmd), flagsComp);
                addFlag(compile_cmd, sizeof(compile_cmd), " -o ");
                addFlag(compile_cmd, sizeof(compile_cmd), filename);
            } else {
                snprintf(compile_cmd, sizeof(compile_cmd),
                         "gcc %s.s%s -o %s", filename, flagsComp, filename);
            }

            if (flags.verbose) printf("Compiling: %s\n", compile_cmd);
            res = system(compile_cmd);
            if (res != 0) {
                fprintf(stderr, "Compilation failed!\n");
                ok = false;
            }
        }
    } else if (ok && hasCpp) {
        // C and C++ must be compiled separately, then linked with g++.
        snprintf(c_obj, sizeof(c_obj), "%s.awl_c.o", filename);

        snprintf(compile_cmd, sizeof(compile_cmd),
                 "gcc -w %s.c%s -c -o %s", filename, flagsComp, c_obj);
        if (flags.verbose) printf("Compiling C: %s\n", compile_cmd);
        res = system(compile_cmd);
        if (res != 0) {
            fprintf(stderr, "Compilation failed!\n");
            ok = false;
        }

        if (ok) {
            snprintf(compile_cmd, sizeof(compile_cmd),
                     "g++ -w %s", c_obj);
            for (int i = 0; i < cppFileCount; i++) {
                addFlag(compile_cmd, sizeof(compile_cmd), " ");
                addFlag(compile_cmd, sizeof(compile_cmd), cpp_obj[i]);
            }
            addFlag(compile_cmd, sizeof(compile_cmd), flagsComp);
            addFlag(compile_cmd, sizeof(compile_cmd), " -o ");
            addFlag(compile_cmd, sizeof(compile_cmd), filename);

            if (flags.verbose) printf("Linking: %s\n", compile_cmd);
            res = system(compile_cmd);
            if (res != 0) {
                fprintf(stderr, "Linking failed!\n");
                ok = false;
            }
        }
    } else if (ok) {
        // No C++ imports: keep the fast one-step C build.
        snprintf(compile_cmd, sizeof(compile_cmd),
                 "gcc -w %s.c%s -o %s", filename, flagsComp, filename);

        if (flags.verbose) printf("Compiling: %s\n", compile_cmd);
        res = system(compile_cmd);
        if (res != 0) {
            fprintf(stderr, "Compilation failed!\n");
            ok = false;
        }
    }

    // Remove temporary object files.
    if (hasCpp) {
        for (int i = 0; i < cppFileCount; i++) {
            char rm[1200];
            snprintf(rm, sizeof(rm), "rm -f %s", cpp_obj[i]);
            system(rm);
        }
    }
    if (c_obj[0] != '\0') {
        char rm[1200];
        snprintf(rm, sizeof(rm), "rm -f %s", c_obj);
        system(rm);
    }

    if (flags.verbose && ok) printf("Binary: %s\n", filename);

    //delete .c if keepc = false
    if (!flags.keepC) {
        char rm[1200];
        snprintf(rm, sizeof(rm), "rm %s.c", filename);
        system(rm);
    }
}
