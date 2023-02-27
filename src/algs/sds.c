#include <stdlib.h>
#include <string.h>

#include "sds.h"

#define SDS_REALLOC_MAX 1024

sds sdsnew(char *init) {
    int len = strlen(init);

    sdshdr_t *sh = malloc(sizeof(*sh) + len + 1);
    sh->len = len;
    sh->free = 0;
    memcpy(sh->buf, init, len);
    sh->buf[len] = '\0';

    return sh->buf;
}

sds sdsempty() {
    sdshdr_t *sh = malloc(sizeof(*sh) + 1);
    sh->len = 0;
    sh->free = 0;
    sh->buf[0] = '\0';
    return sh->buf;
}

void sdsfree(sds s) {
    sdshdr_t *sh = (void *)s - sizeof(*sh);
    free(sh);
}

inline int sdslen(const sds s) {
    sdshdr_t *sh = (void *)s - sizeof(*sh);
    return sh->len;
}

inline int sdsavail(sds s) {
    sdshdr_t *sh = (void *)s - sizeof(*sh);
    return sh->free;
}

sds sdscatlen(sds s, char *t, int len) {
    s = sdsMakeRoomFor(s, len);

    sdshdr_t *sh = (void *)s - sizeof(*sh);
    memcpy(sh->buf + sh->len, t, len);
    sh->len += len;
    sh->free -= len;
    sh->buf[sh->len] = '\0';

    return s;
}

sds sdscat(sds s, char *t) {
    int len = strlen(t);
    return sdscatlen(s, t, len);
}

sds sdscatsds(sds s, sds t) {
    int len = sdslen(t);
    return sdscatlen(s, t, len);
}

sds sdsMakeRoomFor(sds s, int addlen) {
    sdshdr_t *sh = (void *)s - sizeof(*sh);

    if (sh->free >= addlen)
        return s;

    int newlen = sh->len + addlen;
    if (newlen <= SDS_REALLOC_MAX)
        newlen += newlen;
    else
        newlen += SDS_REALLOC_MAX;

    sh = realloc(sh, sizeof(*sh) + newlen + 1);
    sh->free = newlen - sh->len;

    return sh->buf;
}
