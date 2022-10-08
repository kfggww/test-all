#ifndef _BUDDY_H_
#define _BUDDY_H_

#include <stdlib.h>

void buddy_init();
void buddy_fini();
void *buddy_alloc(size_t size);
int buddy_free(void *p);
void buddy_info();

#endif