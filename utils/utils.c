#include "utils.h"

CompilerFlags flags = {0};
bool compileThis = false;
char pythonFlags[512] = {0};

void printVersion(void) {
    printf(CYAN"AWL Compiler v0.8.1\n");
    printf(WHITE"Language: AWL (Awent0_o Language)\n");
}

void printHelp(const char *prog) {
    printf(CYAN_BRIGHT"Usage: " WHITE"%s <file.awl> [options]\n\n", prog);
    printf(BLUE"Options:\n");
    printf("  -h, --help       Show this help\n");
    printf("  --version        Show version\n");
    printf("  -v               Verbose output\n");
    printf("  -c, --compile    Compile to c\n");
    printf("  -k, --keepc      Keep generated .c file\n");
    printf("  -S, --asm        Emit assembly (.s file)\n");
    printf("  -t, --time       Show compilation time\n");
    printf("  -o               Сompile into your filename\n");
    printf("\n");
    printf("GCC flags (passed to backend):\n");
    printf("  -g               Debug information\n");
    printf("  -O1              Optimization level 1\n");
    printf("  -O2              Optimization level 2\n");
    printf("  -O3              Optimization level 3\n");
    printf("  -Wall            All warnings\n");
    printf("  -Wextra          Extra warnings\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s main.awl              Compile main\n", prog);
    printf("  %s main.awl -c           Generate main.c\n", prog);
    printf("  %s main.awl -o test      Compile test\n", prog);
    printf("  %s main.awl -c -g -O2    Compile with debug and optimization\n", prog);
    printf("  %s main.awl -S           Generate assembly\n", prog);
    printf("  %s main.awl -c -k        Compile and keep .c file\n", prog);
}

/*
 * parseArgs — parse args in any order
 *
 * Підтримує:
 *   ./awl main.awl -c -g        ← file first one
 *   ./awl -c main.awl -g        ← file in middle
 *   ./awl -c -g main.awl        ← file last one
 */
void parseArgs(int argc, char *argv[]) {
    memset(&flags, 0, sizeof(CompilerFlags));

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];

        //input file — any argument ending in .awl 
        if (strlen(arg) > 4 &&
            strcmp(arg + strlen(arg) - 4, ".awl") == 0) {
            strncpy(flags.inputFile, arg, sizeof(flags.inputFile) - 1);

            // output name = file name .awl 
            strncpy(flags.outputName, arg, sizeof(flags.outputName) - 1);
            char *dot = strrchr(flags.outputName, '.');
            if (dot) *dot = '\0';
            continue;
        }

        // FLAGS
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            flags.showHelp = true;
        } else if (strcmp(arg, "--version") == 0) {
            flags.showVersion = true;
        } 
        else if (strcmp(arg, "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, RED "Error: " WHITE "-o requires an output filename\n");
                compileThis = false;
            }

            strncpy(flags.outputName, argv[++i], sizeof(flags.outputName) - 1);
            flags.outputName[sizeof(flags.outputName) - 1] = '\0';
        }
        else if (strcmp(arg, "-v") == 0) {
            flags.verbose = true;
        } else if (strcmp(arg, "-c") == 0 || strcmp(arg, "--onlyC") == 0) {
            flags.compileToC = true;
        } else if (strcmp(arg, "-k") == 0 || strcmp(arg, "--keepc") == 0) {
            flags.keepC = true;
        } else if (strcmp(arg, "-S") == 0 || strcmp(arg, "--asm") == 0) {
            flags.emitAsm = true;
            flags.compileToC = false; // -S need to compile
        } else if (strcmp(arg, "-t") == 0 || strcmp(arg, "--time") == 0) {
            flags.showTime = true;
        } else if (strcmp(arg, "-g") == 0) {
            flags.debugInfo = true;
        } else if (strcmp(arg, "-O1") == 0) {
            flags.optimize1 = true;
        } else if (strcmp(arg, "-O2") == 0) {
            flags.optimize2 = true;
        } else if (strcmp(arg, "-O3") == 0) {
            flags.optimize3 = true;
        } else if (strcmp(arg, "-Wall") == 0) {
            flags.wallWarnings = true;
        } else if (strcmp(arg, "-Wextra") == 0) {
            flags.wextra = true;
        } else {
            fprintf(stderr, RED"Unknown option: " WHITE"%s\n", arg);
            fprintf(stderr, "Run with -h for help\n");
        }
    }
}


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

void initFlags() {
    //python flag
    FILE *f = popen("python3-config --includes --ldflags --embed 2>/dev/null", "r");
    if (f) {
        fgets(pythonFlags, sizeof(pythonFlags), f);
        pclose(f);
        pythonFlags[strcspn(pythonFlags, "\n")] = '\0';
    }
}

char *_strdup(const char *src) {
    if (src == NULL)
        return NULL;

    size_t len = strlen(src) + 1;
    char *dst = malloc(len);

    if (dst == NULL)
        return NULL;

    memcpy(dst, src, len); 
    return dst;
}
