#include <Arduino.h>
#include <esp_log.h>
#include "ctime"

int hexdump(char* dest, const void* src, size_t size) {
  int dump_size = size > 256 ? 256 : size;
  for (int i = 0; i < dump_size; i++) {
    sprintf(dest + i * 3, "%02X ", ((char*)src)[i]);
  }

  dest[dump_size * 3] = 0x00;
  return dump_size;
}

bool bounce(time_t* timekeeper, int bounce_ms = 1000, bool renew = false) {
  long now = millis();
  if (*timekeeper < now) {
    *timekeeper = now + bounce_ms;
    return true;
  }
  if (renew) {
    *timekeeper = now + bounce_ms;
  }
  return false;
}
