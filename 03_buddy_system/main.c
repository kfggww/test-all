#include <stdlib.h>
#include <stdio.h>
#include "buddy.h"

int main(int argc, char **argv)
{

    buddy_init();

    size_t sizes[] = {2000, 1000, 500, 200};
    const int N = sizeof(sizes) / sizeof(size_t);
    void *bufs[N];

    for (int n = 0; n < 2; ++n)
    {
        printf("\n\n=================== test #%d: ===================\n\n", n);
        printf("buddy alloc result:\n\n");
        for (int i = 0; i < N; ++i)
        {
            bufs[i] = buddy_alloc(sizes[i]);
            printf("#%d: %p\n", i, bufs[i]);
        }

        printf("\n\nblock info before free:\n\n");
        buddy_info();

        for (int i = 0; i < N; ++i)
        {
            buddy_free(bufs[i]);
        }

        printf("\n\nblock info after free:\n\n");
        buddy_info();
    }

    buddy_fini();
    return 0;
}