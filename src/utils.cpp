#include <cstdio>
#include <esp_log.h>

int hexdump(char *dest, const void *src, size_t size) {
  int dump_size = size > 256 ? 256 : size;
  for (int i = 0; i < dump_size; i++) {
    sprintf(dest + i * 3, "%02X ", ((char *)src)[i]);
  }

  dest[dump_size * 3] = 0x00;
  return dump_size;
}
