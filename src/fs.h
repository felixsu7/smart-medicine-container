#ifndef FILES_H
#define FILES_H

#include <cstddef>

int load_file(const char *filename, char *dest, size_t dest_size);
int save_file(const char *filename, char *src, size_t src_size);
int setup_fs();

#endif
