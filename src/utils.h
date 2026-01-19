#ifndef UTILS_H
#define UTILS_H

#include <cstdio>

#include "mutex"

static std::mutex fs_mutex;

// Prints a hexidecimal representation into dest from src with specified size.
// dest must be 3 times larger than size, the null terminator is accounted.
// Currently size will be limited to 256.
int hexdump(char* dest, const void* src, size_t size);

// Returns true if associated functionality should be now run and false
// otherwise. Basically for limiting function calls per a period of time. renew
// will update the timekeeper regardless of state, good for button pressses.
bool bounce(time_t* timekeeper, int bounce_ms = 1000, bool renew = false);

#endif
