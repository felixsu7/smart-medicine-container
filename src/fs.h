#ifndef FILES_H
#define FILES_H

#include <cstddef>

int setup_fs(void);

int fs_format(void);
int fs_load_file(const char *filename, char *dest, size_t dest_size);
int fs_save_file(const char *filename, char *src, size_t src_size);

#endif
