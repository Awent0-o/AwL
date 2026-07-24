#include "utils.h"

bool compileThis = true;
char pythonFlags[512] = {0};
char luaFlags[128] = {0};

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