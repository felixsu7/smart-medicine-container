#ifndef UTILS_H
#define UTILS_H

#include <cstdio>

// Prints a textual representation into dest from src with specified size. dest
// must be 3 times larger than size, the null terminator is accounted. Currently
// size will be limited to 256.
int hexdump(char *dest, const void *src, size_t size);

#endif
