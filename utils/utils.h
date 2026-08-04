#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>


//FLAG COMPILE
typedef struct {
    // INPUT/OUPTUP
    char inputFile[512];   // main.awl
    char outputName[512];  // name output file without ext

    // MODE
    bool compileToC;    // -c / --compile  → gen only c
    bool keepC;         // -k / --keepc    → dont rm .c 
    bool emitAsm;       // -S / --asm      → gen in .s file
    bool showTime;      // -t / --time     → show compile time
    bool verbose;       // -v              → compilation details
    bool showHelp;      // -h / --help     → help
    bool showVersion;   // --version       → version

    // GCC FLAG
    bool debugInfo;     // -g              → debug 
    bool optimize1;     // -O1             → optimizetion level
    bool optimize2;     // -O2
    bool optimize3;     // -O3
    bool wallWarnings;  // -Wall
    bool wextra;        // -Wextra
} CompilerFlags;

//UTILS VAR
extern CompilerFlags flags;
extern bool hasPython;
extern bool hasCImport;
extern bool compileThis;
extern char pythonFlags[512];

//COLOR DEFINE
#define BLUE "\033[34m"
#define WHITE "\033[37m"
#define RED "\033[31m"
#define CYAN "\033[36m"
#define MAGENTA "\033[35m"
#define CYAN_BRIGHT "\033[96m"


//UTILS FUNC
void parseArgs(int argc, char *argv[]);
void printHelp(const char *programName);
void printVersion(void);
char *readFile(const char *path, size_t *outLen);
const char *get_file_ext(const char *filename);
void initFlags();
char *_strdup(const char *src); //write my own strdup because strdup from string.h dont work 

#endif