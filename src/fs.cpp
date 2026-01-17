#include "./fs.h"
#include "./utils.h"
#include "LittleFS.h"
#include <cstddef>

static const bool DEBUG_DUMP = true;
static const char *FILES_TAG = "files";

int fs_load_file(const char *filename, char *dest, size_t dest_size) {
  File file = LittleFS.open(filename, FILE_READ);
  if (!file || file.isDirectory()) {
    ESP_LOGE(FILES_TAG, "opening %s for reading.", filename);
    return -1;
  }

  size_t res = file.readBytes(dest, dest_size);
  if (res != dest_size) {
    ESP_LOGE(FILES_TAG, "reading from %s, res %d, size %d", filename, res,
             dest_size);
    return -2;
  };

  // FIXME: sloppy code
  if (DEBUG_DUMP) {
    char dump[256 * 3 + 1];
    int res = hexdump(dump, dest, dest_size);

    ESP_LOGD(FILES_TAG, "first %d bytes dump of %s: %s", res, filename, dump);
  }

  file.close();

  return 0;
}

int fs_save_file(const char *filename, char *src, size_t src_size) {
  File file = LittleFS.open(filename, FILE_WRITE);
  if (!file || file.isDirectory()) {
    ESP_LOGE(FILES_TAG, "opening %s for writing", filename);
    return -1;
  }

  if (size_t res = file.write((uint8_t *)src, src_size); res != src_size) {
    ESP_LOGE(FILES_TAG, "writing to %s, res %d, size %d", filename, res,
             src_size);
    return -2;
  };

  file.close();

  return 0;
}

int fs_format(void) {
  assert(LittleFS.format());
  return 0;
}

int setup_fs(void) {
  if (!LittleFS.begin()) {
    assert(fs_format());
    esp_restart();
  }

  ESP_LOGI(FILES_TAG, "fs size: %ld/%ld\n", LittleFS.usedBytes(),
           LittleFS.totalBytes());

  return 0;
}
