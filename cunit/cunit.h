#ifndef CUNIT_H
#define CUNIT_H

#include <stdio.h>

typedef void (*test_case_fn)();

#define DECL_TEST_SUIT(suit_name) test_case_fn cunit_suit_##suit_name[];

#define TEST_SUIT_BEGIN(suit_name) test_case_fn cunit_suit_##suit_name[] = {
#define ADD_TEST(suit_name, test_name) suit_name##test_name,
#define TEST_SUIT_END                                                          \
    NULL                                                                       \
    }                                                                          \
    ;

#define TEST_CASE(suit_name, test_name) void suit_name##test_name()

#define RUN_ALL(suit_name) cunit_run_all(cunit_suit_##suit_name)

static inline void cunit_run_all(test_case_fn *tests) {
    if (tests == NULL)
        return;

    printf("CUNIT begin runing all tests ...\n\n");
    int i = 0;
    while (tests[i] != NULL) {
        tests[i]();
        i++;
    }
    printf("CUNIT end runing all tests.\n\n");
}

#endif