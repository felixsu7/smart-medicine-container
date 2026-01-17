#include "./clock.h"
#include "./config.h"
#include "WiFi.h"
#include <uRTCLib.h>

static const char *TAG = "clock";

uRTCLib rtc(0x68);

int clock_sync_ntp(void) {
  int gmt_offset = DEFAULT_GMT_OFFSET_SECS;
  int daylight_offset = DEFAULT_DAYLIGHT_OFFSET_SECS;

  if (WiFi.status() != WL_CONNECTED) {
    ESP_LOGE(TAG, "no wifi for ntp");
    return 1;
  }

  struct tm timeInfo;
  for (int i = 5; i > 0; i--) {
    configTime(gmt_offset, daylight_offset, NTP_SERVER_PRI, NTP_SERVER_SEC,
               NTP_SERVER_TRI);

    delay(5000);

    time_t now;
    time(&now);
    localtime_r(&now, &timeInfo);

    if (timeInfo.tm_year < 126) {
      ESP_LOGE(TAG, "year is not 2026 and onwards (got %d)", timeInfo.tm_year);
    } else {
      break;
    }
  }

  if (timeInfo.tm_year < 126) {
    ESP_LOGE(TAG, "sync from ntp failed entirely!");
    return -1;
  }

  char buf[20];
  strftime(buf, 20, "%Y-%m-%d %H:%M:%S", &timeInfo);

  ESP_LOGI(TAG, "current time from ntp: %s", buf);

  return 0;
}

int setup_clock(void) {
  assert(URTCLIB_WIRE.begin());

  rtc.set_model(URTCLIB_MODEL_DS3231);

  assert(rtc.refresh());
  assert(rtc.enableBattery());
  assert(!rtc.getEOSCFlag());

  if (rtc.lostPower()) {
    ESP_LOGW(TAG, "rtc module lost power");
    if (clock_sync_ntp() != 0) {
      return -1;
    };

    struct tm now;
    if (clock_get(&now) != 0) {
      return -2;
    };

    rtc.set(now.tm_sec, now.tm_min, now.tm_hour, now.tm_wday, now.tm_mday,
            now.tm_mon, now.tm_year - 100);
    if (!rtc.refresh()) {
      return -3;
    }

    rtc.lostPowerClear();
  } else {
    struct tm now;
    now.tm_sec = rtc.second();
    now.tm_min = rtc.minute();
    now.tm_hour = rtc.hour();
    now.tm_mday = rtc.day();
    now.tm_mon = rtc.month();
    now.tm_year = rtc.year() + 100;

    time_t now_sec = mktime(&now);
    struct timeval val = {.tv_sec = now_sec};
    struct timezone tz = {.tz_minuteswest = DEFAULT_GMT_OFFSET_SECS / 60};
    if (int err = settimeofday(&val, &tz); err != 0) {
      return err;
    }

    struct tm test;

    assert(clock_get(&test) == 0);
    assert(test.tm_year >= 126);

    char buf[20];

    strftime(buf, 20, "%Y-%m-%d %H:%M:%S", &test);
    ESP_LOGI(TAG, "current time from rtc: %s", buf);
  }

  return 0;
}

int clock_get(struct tm *dest_tm) {
  for (int i = 5; i > 0; i--) {
    time_t now;
    time(&now);
    localtime_r(&now, dest_tm);

    if (dest_tm->tm_year < 126) {
      ESP_LOGE(TAG, "year is not 2026 and onwards (got %d)", dest_tm->tm_year);
    } else {
      return 0;
    }
  }

  ESP_LOGE(TAG, "failed getting local time entirely");
  return -1;
}
