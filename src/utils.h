#ifndef UTILS_H
#define UTILS_H

#include <cstdio>

int hexdump(char *dest, void *src, size_t src_size);
int printmem(const char *msg, void *src, size_t size);

#endif
