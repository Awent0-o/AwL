#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

extern bool hasPython;

extern bool hasCImport;

extern bool compileThis;
extern char pythonFlags[512];

char *readFile(const char *path, size_t *outLen);

const char *get_file_ext(const char *filename);

void initFlags();

char *_strdup(const char *src);

#endif