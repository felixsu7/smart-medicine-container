#include <cstdio>
#include <esp_log.h>

int hexdump(char *dest, void *src, size_t src_size) {
  int dump_size = src_size > 256 ? 256 : src_size;
  for (int i = 0; i < dump_size; i++) {
    sprintf(dest + i * 3, "%02X ", ((char *)src)[i]);
  }
  return dump_size;
}
