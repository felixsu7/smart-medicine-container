#include "./fs.h"
#include "./utils.h"
#include "LittleFS.h"
#include <cstddef>

static const bool DEBUG_DUMP = true;

static const char *TAG = "fs";

int Filesystem::read(const char *path, char *dest, size_t size) {
  File file = LittleFS.open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    ESP_LOGE(TAG, "opening %s for reading.", path);
    return -1;
  }

  size_t res = file.readBytes(dest, size);
  if (res != size) {
    ESP_LOGE(TAG, "reading from %s, res %d, size %d", path, res, size);
    return -2;
  };

  // FIXME: sloppy code
  if (DEBUG_DUMP) {
    char dump[256 * 3 + 1];
    int res = hexdump(dump, dest, size);

    ESP_LOGD(TAG, "first %d bytes dump of %s: %s", res, path, dump);
  }

  file.close();

  return 0;
}

int Filesystem::write(const char *path, const char *src, size_t size) {
  File file = LittleFS.open(path, FILE_WRITE);
  if (!file || file.isDirectory()) {
    ESP_LOGE(TAG, "opening %s for writing", path);
    return -1;
  }

  if (size_t res = file.write((uint8_t *)src, size); res != size) {
    ESP_LOGE(TAG, "writing to %s, res %d, size %d", path, res, size);
    return -2;
  };

  file.close();

  return 0;
}

bool Filesystem::format(void) {
  assert(LittleFS.format());
  return true;
}

Filesystem::Filesystem(void) {
  if (!LittleFS.begin()) {
    assert(format());
    esp_restart();
  }

  ESP_LOGI(TAG, "fs size: %ld/%ld\n", LittleFS.usedBytes(),
           LittleFS.totalBytes());
}
