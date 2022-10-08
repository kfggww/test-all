#include <stdlib.h>
#include <stdio.h>
#include "buddy.h"

int main(int argc, char **argv)
{

    buddy_init();

    size_t sizes[] = {2000, 1000, 500, 200};
    for (int i = 0; i < sizeof(sizes) / sizeof(size_t); ++i)
    {
        void *pbuf = buddy_alloc(sizes[i]);
        printf("#%d: %p\n", i, pbuf);
    }

    buddy_info();
    buddy_fini();
    return 0;
}