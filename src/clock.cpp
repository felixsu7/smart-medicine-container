#include "./clock.h"
#include "./config.h"
#include "WiFi.h"

static const char *CLOCK_TAG = "clock";

int setup_clock(void) {
  int gmt_offset = GMT_OFFSET;
  int daylight_offset = DAYLIGHT_OFFSET;

  // TODO: DS3231
  if (WiFi.status() != WL_CONNECTED) {
    ESP_LOGE(CLOCK_TAG, "no wifi for ntp");
    return 1;
  }

  struct tm timeInfo;
  int retries = 0;

  configTime(gmt_offset, daylight_offset, NTP_SERVER_PRI, NTP_SERVER_SEC,
             NTP_SERVER_TRI);

  time_t now;
  time(&now);
  localtime_r(&now, &timeInfo);

  if (timeInfo.tm_year < 125) {
    ESP_LOGE(CLOCK_TAG, "year is not 2025 and onwards (got %d)",
             timeInfo.tm_year);
    return 1;
  }

  char buf[20];

  strftime(buf, 20, "%Y-%m-%d %H:%M:%S", &timeInfo);

  ESP_LOGI(CLOCK_TAG, "current time: %s", buf);

  return 0;
}
