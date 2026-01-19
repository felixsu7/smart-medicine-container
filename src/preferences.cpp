#include "preferences.h"
#include "LittleFS.h"
#include "cassert"
#include "config.h"
#include "cstring"
#include "utils.h"

static const char* TAG = "preferences";

int DevicePreferences::setup(void) {
  // TODO DEBUG
  if (int err = load_from_fs(); err == -2) {
    assert(LittleFS.format());
    esp_restart();
  } else if (err < -2) {
    ESP_LOGE(TAG, "err is %d", err);
    assert(false);
  };

  return 0;
}

int DevicePreferences::load_from_fs(void) {
  fs_mutex.lock();
  File file = LittleFS.open(PREFERENCES_PATH, FILE_READ);
  assert(!file.isDirectory());
  if (!file) {
    // it needs to be closed because it would collide with the alarm saving
    // mechanism, applies for anything FS.
    file.close();
    fs_mutex.unlock();
    ESP_LOGW(TAG, "no saved data at %s, ignoring", PREFERENCES_PATH);
    return -1;
  }

  size_t res = file.readBytes((char*)this, sizeof(DevicePreferences));
  if (res != sizeof(DevicePreferences)) {
    file.close();
    fs_mutex.unlock();
    ESP_LOGE(TAG, "reading from %s, res %d, size %d", PREFERENCES_PATH, res,
             sizeof(DevicePreferences));
    return -2;
  };

  file.close();
  fs_mutex.unlock();

  // TODO DEBUG
  char dump[256 * 3 + 1];
  int dump_res = hexdump(dump, this, sizeof(DevicePreferences));
  ESP_LOGD(TAG, "first %d bytes dump of %s: %s", 256, PREFERENCES_PATH, dump);

  if (version != PREFERENCES_VERSION) {
    ESP_LOGE(TAG, "unsupported version: %02X", version);
    return -3;
  }

  // TODO macros?
  // memcpy(web_password, data + offsetof(DevicePreferences, web_password),
  //        sizeof(web_password));
  // for (int i = 0; i < sizeof(wifi_configs) / sizeof(wifi_configs[0]); i++) {
  //   memcpy(wifi_configs + i * sizeof(WiFiConfig),
  //          data + offsetof(DevicePreferences, wifi_configs) +
  //              i * sizeof(WiFiConfig),
  //          sizeof(WiFiConfig));
  // }
  //
  // memcpy(&gmt_offset, data + offsetof(DevicePreferences, gmt_offset),
  //        sizeof(gmt_offset));
  // memcpy(&daylight_offset, data + offsetof(DevicePreferences,
  // daylight_offset),
  //        sizeof(daylight_offset));
  // memcpy(notify_url, data + offsetof(DevicePreferences, notify_url),
  //        sizeof(notify_url));

  return 0;
}

int DevicePreferences::save_into_fs(void) {
  fs_mutex.lock();
  File file = LittleFS.open(PREFERENCES_PATH, FILE_WRITE);

  assert(file && !file.isDirectory());

  assert(file.write((const uint8_t*)this, sizeof(DevicePreferences)) ==
         sizeof(DevicePreferences));

  // TODO again, macros
  // assert(file.write(version) == 1);
  // assert(file.write((const uint8_t *)web_password, sizeof(web_password)) ==
  //        sizeof(web_password));
  // for (int i = 0; i < sizeof(wifi_configs) / sizeof(wifi_configs[0]); i++) {
  //   assert(file.write((const uint8_t *)&wifi_configs[i], sizeof(WiFiConfig))
  //   ==
  //          sizeof(WiFiConfig));
  // }
  // assert(file.write((const uint8_t *)&gmt_offset, sizeof(gmt_offset)) ==
  //        sizeof(gmt_offset));
  // assert(file.write((const uint8_t *)&daylight_offset,
  //                   sizeof(daylight_offset)) == sizeof(daylight_offset));
  // assert(file.write((const uint8_t *)notify_url, sizeof(notify_url)) ==
  //        sizeof(notify_url));

  file.close();
  fs_mutex.unlock();

  return 0;
}
