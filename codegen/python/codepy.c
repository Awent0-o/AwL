#include "codepy.h"

PyModule pyModules[128];
char pyModuleNames[16][64];
int pyModuleCount = 0;

//safe py module
void addPyModule(const char *name, PyModule mod) {
    strncpy(pyModuleNames[pyModuleCount], name, 63);
    pyModules[pyModuleCount] = mod;
    pyModuleCount++;
}

//we determine the type from the type hint or the parameter name
static DataType guessType(const char *hint) {
    if (!hint || strlen(hint) == 0) return TYPE_INT;  // defualt
    if (strcmp(hint, "int") == 0)   return TYPE_INT;
    if (strcmp(hint, "str") == 0)   return TYPE_STRING;
    if (strcmp(hint, "float") == 0) return TYPE_FLOAT;
    if (strcmp(hint, "bool") == 0)  return TYPE_BOOL;
    return TYPE_INT;  // unknown type -> int
}

// parse param name: int or name
static DataType parseParam(const char *param) {
    const char *colon = strchr(param, ':');
    if (!colon) return TYPE_INT;

    char hint[32] = {0};
    const char *p = colon + 1;
    while (*p == ' ') p++;
    int i = 0;
    while (*p && *p != ',' && *p != ')' && *p != ' ' && i < 31) {
        hint[i++] = *p++;
    }
    return guessType(hint);
}

// parse return type
static DataType parseReturnType(const char *line) {
    const char *arrow = strstr(line, "->");
    if (!arrow) return TYPE_INT;  // default int

    const char *p = arrow + 2;
    while (*p == ' ') p++;

    char hint[32] = {0};
    int i = 0;
    while (*p && *p != ':' && *p != ' ' && i < 31) {
        hint[i++] = *p++;
    }
    return guessType(hint);
}

PyModule parsePyFile(const char *path) {
    PyModule mod;
    memset(&mod, 0, sizeof(PyModule));
    const char *base = strrchr(path, '/');
    #ifdef _WIN32
    const char *base2 = strrchr(path, '\\');
    if (!base || (base2 && base2 > base)) base = base2;
    #endif

    base = base ? base + 1 : path;

    mod.name = _strdup(base);

    char *dot = strrchr(mod.name, '.');
    if (dot) *dot = '\0';

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Cannot open Python file: %s\n", path);
        return mod;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        //we are looking for lines starting with "def ".
        char *p = line;
        while (*p == ' ') p++;
        if (strncmp(p, "def ", 4) != 0) continue;
        p += 4; 

        PyFunc func;
        memset(&func, 0, sizeof(PyFunc));

        //name func
        char nameBuf[256] = {0};
        int i = 0;
        while (*p && *p != '(' && i < 255) {
            nameBuf[i++] = *p++;
        }
        nameBuf[i] = '\0';

        //after parse:
        func.name = _strdup(nameBuf);  //dynamic allocation

        //params ( i )
        if (*p == '(') p++;
        char params_str[256] = {0};
        int pi = 0;
        while (*p && *p != ')' && pi < 255) {
            params_str[pi++] = *p++;
        }

        //split the parameters by comma
        char *param = strtok(params_str, ",");
        while (param && func.paramCount < MAX_PY_PARAMS) {
            while (*param == ' ') param++;
            //skip self
            if (strncmp(param, "self", 4) != 0) {
                func.paramTypes[func.paramCount++] = parseParam(param);
            }
            param = strtok(NULL, ",");
        }

        //return type
        func.returnType = parseReturnType(line);

        mod.funcs[mod.count++] = func;
    }

    fclose(f);
    return mod;
}

PyFunc *findPyFunc(const char *modname, const char *funcname) {

    for (int m = 0; m < pyModuleCount; m++) {
         printf("module[%d] = '%s'\n", m, pyModuleNames[m]);
         printf("module[%d] = '%s', funcs = %d\n",
           m,
           pyModuleNames[m],
           pyModules[m].count);
        if (strcmp(pyModuleNames[m], modname) == 0) { 
            for (int f = 0; f < pyModules[m].count; f++) {
                if (strcmp(pyModules[m].funcs[f].name, funcname) == 0) {
                    return &pyModules[m].funcs[f];
                }
            }
        }
    }
    return NULL;
}