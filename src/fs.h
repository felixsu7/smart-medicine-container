#ifndef FILES_H
#define FILES_H

#include <cstddef>

int setup_fs();

int fs_format();
int fs_load_file(const char *filename, char *dest, size_t dest_size);
int fs_save_file(const char *filename, char *src, size_t src_size);

#endif
