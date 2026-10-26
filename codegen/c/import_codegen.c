#include "codegen_internal.h"

void genImport(Node *n, FILE *out) {
    const char *path = n->text;
    size_t len = strlen(path);

    //import ".py"
    if (len > 3 && strcmp(path + len - 3, ".py") == 0) {
        if (!hasPython) {

            hasPython = true;
            fprintf(out, "#include <stdarg.h>\n");
            fprintf(out, "%s", AWL_PY_RUNTIME);
        }
        strcpy(pyFiles[pyFileCount++], path);

        //parse file py and memorize func
        PyModule mod = parsePyFile(path);

        pyModules[pyModuleCount] = mod;
        snprintf(pyModuleNames[pyModuleCount], sizeof(pyModuleNames[pyModuleCount]), "%s", mod.name);
        pyModuleCount++;
        return;
    }

    //import ".awl"
    if (len > 4 && strcmp(path + len - 4, ".awl") == 0) {
        // parse awl
        size_t fileLen;
        char *src = readFile(path, &fileLen);
        if (!src) {
            fprintf(stderr, "Cannot open import: %s\n", path);
            return;
        }

        TokenList tokens = tokenize(src, fileLen);
        Parser parser = { .tokens = &tokens, .pos = 0 };
        Node *importedAst = parseProgram(&parser);

        // gen only func
        for (int i = 0; i < importedAst->stmtCount; i++) {
            if (importedAst->statements[i]->type == NODE_FUNC_DECL) {
                genFuncDecl(importedAst->statements[i], out);
            }
        }

        free(src);
        free(tokens.tokens);
        return;
    }

    // else - C lib
    // "math" -> #include <math.h>
    // "myheader.h" -> #include "myheader.h"
    if (len > 2 && strcmp(path + len - 2, ".h") == 0) {
        fprintf(out, "#include \"%s\"\n", path);
    } else {
        fprintf(out, "#include <%s.h>\n", path);
    }
}