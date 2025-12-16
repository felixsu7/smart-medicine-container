#include "./fs.h"
#include "LittleFS.h"

static const bool DEBUG_DUMP = true;
static const char *FILES_TAG = "files";

int load_file(const char *filename, char *dest, size_t dest_size) {
  File file = LittleFS.open(filename, FILE_READ);
  if (!file || file.isDirectory()) {
    ESP_LOGE(FILES_TAG, "opening %s for reading.", filename);
    return 1;
  }

  if (size_t res = file.readBytes(dest, dest_size); res != dest_size) {
    ESP_LOGE(FILES_TAG, "reading from %s, res %d, size %d", filename, res,
             dest_size);
    return 2;
  };

  // FIXME: sloppy code
  if (DEBUG_DUMP) {
    int dump_size = dest_size > 256 ? 256 : dest_size;
    char dump[dump_size * 3 + 1];
    for (int i = 0; i < dump_size; i++) {
      sprintf(dump + i * 3, "%02X ", ((char *)dest)[i]);
    }

    ESP_LOGD(FILES_TAG, "first %d bytes dump of %s: %s", dump_size, filename,
             dump);
  }

  file.close();

  return 0;
}

int save_file(const char *filename, char *src, size_t src_size) {
  File file = LittleFS.open(filename, FILE_WRITE);
  if (!file || file.isDirectory()) {
    ESP_LOGE(FILES_TAG, "opening %s for writing", filename);
    return 1;
  }

  if (size_t res = file.write((uint8_t *)src, src_size); res != src_size) {
    ESP_LOGE(FILES_TAG, "writing to %s, res %d, size %d", filename, res,
             src_size);
    return 2;
  };

  file.close();

  return 0;
}

int setup_fs() {
  if (!LittleFS.begin()) {
    ESP_LOGE(FILES_TAG, "littlefs mount");
    return 1;
  }

  ESP_LOGI(FILES_TAG, "fs size: %ld/%ld\n", LittleFS.usedBytes(),
           LittleFS.totalBytes());

  return 0;
}
