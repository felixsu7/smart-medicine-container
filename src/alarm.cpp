#include "./alarm.h"
#include "config.h"
#include "esp32-hal-log.h"
#include "time.h"
#include <Esp.h>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fs.h>

static const char *TAG = "alarm";

int Alarms::load_from_fs(void) {
  int err = Filesystem::read(ALARMS_PATH, (char *)&alarms, sizeof(alarms));

  if (err == -1) {
    ESP_LOGI(TAG, "creating new alarms file...");
    assert(save_into_fs() == 0);
  }

  if (err == -2) {
    ESP_LOGW(TAG, "alarm struct size is different from alarm file");
    return -1;
  }

  if (version != ALARM_VERSION) {
    ESP_LOGE(TAG, "unsupported version: %02X", version);
    return -2;
  }

  return 0;
}

int Alarms::save_into_fs(void) {
  assert(Filesystem ::read(ALARMS_PATH, (char *)&alarms, sizeof(alarms)) == 0);
  return 0;
}

int Alarms::add(const struct Alarm *alarm) {
  for (int i = 0; i < MAX_ALARMS; i++) {
    if (get(i, NULL) == -2) {
      set(i, alarm);
      return i;
    }
  }
  return -1;
};

int Alarms::set(int idx, const struct Alarm *alarm) {
  if (idx < 0 || idx >= MAX_ALARMS) {
    return -1;
  }

  if (alarm == NULL) {
    memset(&alarms[idx], 0, sizeof(alarms[0]));
    return 0;
  }

  memcpy(&alarms[idx], alarm, sizeof(Alarm));
  return 0;
}

int Alarms::get(int idx, struct Alarm *alarm) {
  if (idx < 0 || idx >= MAX_ALARMS) {
    return -1;
  }

  Alarm a = alarms[idx];
  if (a.name[0] == 0x00) {
    return -2;
  }

  if (alarm != NULL) {
    memcpy(alarm, &a, sizeof(Alarm));
  }

  return 0;
}

int Alarms::next_schedule(const struct Alarm *alarm, char today) {
  if (today > 7) {
    return -1;
  }
  if ((alarm->days & 127) == 0x00) {
    return -2;
  }

  ESP_LOGD(TAG, "today %d", today);
  ESP_LOGD(TAG, "days %d", alarm->days);

  // TODO FIXME maybe this could be better.
  for (int i = 0; i < 8; i++) {
    ESP_LOGD(TAG, "i %d", i);
    // Find which days after today matches the alarm days, or something.
    if (alarm->days & (SUNDAY >> ((today + i) % 7))) {
      ESP_LOGD(TAG, "true 1");
      // In case if there is a match today originally, check if the schedule is
      // behind the current second.
      if (alarm->secondMark > (i * 24 * 60 * 60)) {
        ESP_LOGD(TAG, "true 2");
        continue;
      }

      return alarm->secondMark + (i * 24 * 60 * 60);
    }

    ESP_LOGD(TAG, "false 1");
  }

  // UNREACHABLE
  assert(false);
}

time_t Alarms::earliest_alarm(const struct tm *now, struct Alarm *alarm) {
  Alarm *earliest = NULL;
  int earliest_second = INT_MAX;
  for (int idx = 0; idx < MAX_ALARMS; idx++) {
    Alarm test = alarms[idx];
    if (test.name[0] == 0 || (test.days & 127) == 0x00) {
      continue;
    }

    if (earliest == NULL) {
      earliest = &test;
      earliest_second = next_schedule(earliest, now->tm_wday);
      continue;
    }

    int test_second = next_schedule(&test, now->tm_wday);

    if (earliest_second > test_second) {
      earliest = &test;
      earliest_second = test_second;
    }
  }

  if (earliest == NULL) {
    return -1;
  } else {
    // printmem("dest_alarm", dest_alarm, sizeof(dest_alarm));
    // printmem("earliest", earliest, sizeof(earliest));
    memcpy(alarm, earliest, sizeof(Alarm));

    // TODO FIXME extract this into alarm_epoch.
    struct tm now_copy;
    memcpy(&now_copy, now, sizeof(tm));
    now_copy.tm_hour = 0;
    now_copy.tm_min = 0;
    now_copy.tm_sec = 0;

    time_t epoch = mktime(&now_copy);

    return epoch + earliest_second;
  }
}

Alarms::Alarms(void) {
  if (int err = load_from_fs(); err == -2) {
    assert(Filesystem::format());
    esp_restart();
  } else if (err != 0) {
    abort();
  };
  // };
  //

  strcpy(alarms[0].name, "first");
  alarms[0].days |= SUNDAY;
  alarms[0].secondMark = 5;

  strcpy(alarms[1].name, "second");
  alarms[1].days |= MONDAY;
  alarms[1].secondMark = 3;

  strcpy(alarms[2].name, "third");
  alarms[2].days |= MONDAY;

  struct tm now;
  now.tm_sec = 0;
  now.tm_min = 0;
  now.tm_hour = 0;
  now.tm_wday = 0;

  Alarm test = {};
  earliest_alarm(&now, &test);

  ESP_LOGD(TAG, "earliest: %s", test.name);
  ESP_LOGD(TAG, "%s: %d", alarms[0].name, next_schedule(&alarms[0], 0));
  ESP_LOGD(TAG, "%s: %d", alarms[1].name, next_schedule(&alarms[1], 0));
  ESP_LOGD(TAG, "%s: %d", alarms[2].name, next_schedule(&alarms[2], 0));
}
