#ifndef SDS_H
#define SDS_H

typedef char *sds;

typedef struct {
    int len;
    int free;
    char buf[];
} sdshdr_t;

/*sds APIs*/

sds sdsnew(char *init);
sds sdsempty();
void sdsfree(sds s);

int sdslen(sds s);
int sdsavail(sds s);

sds sdscatlen(sds s, char *t, int len);
sds sdscat(sds s, char *t);
sds sdscatsds(sds s, sds t);

sds sdsMakeRoomFor(sds s, int addlen);

#endif