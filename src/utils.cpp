#include <cstdio>
#include <esp_log.h>

int hexdump(char *dest, void *src, size_t size) {
  int dump_size = size > 256 ? 256 : size; // TODO FIXME
  for (int i = 0; i < dump_size; i++) {
    sprintf(dest + i * 3, "%02X ", ((char *)src)[i]);
  }
  return dump_size;
}
