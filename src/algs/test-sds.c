#include <stdio.h>

#include "cutest.h"
#include "sds.h"

CUTEST_SUIT(sdstest)

CUTEST_CASE(sdstest, test1) {
    sds s1 = sdsnew("hello ");
    sds s2 = sdsnew("world");

    s1 = sdscatsds(s1, s2);
    printf("%s\n", s1);

    sdsfree(s1);
    sdsfree(s2);
}