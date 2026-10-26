#include "utils.h"

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
