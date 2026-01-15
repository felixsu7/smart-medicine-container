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

static const char *ALARM_TAG = "alarm";

static AlarmsFile alarms;

int alarms_load(void) {
  int err = fs_load_file(ALARMS_FILENAME, (char *)&alarms, sizeof(alarms));

  if (err == -1) {
    ESP_LOGI(ALARM_TAG, "creating new alarms file...");
    assert(alarms_save() == 0);
  }

  if (err == -2) {
    ESP_LOGW(ALARM_TAG, "alarm struct size is different from alarm file");
    return -1;
  }

  if (alarms.version != ALARM_VERSION) {
    ESP_LOGE(ALARM_TAG, "unsupported version: %02X", alarms.version);
    return -2;
  }

  return 0;
}

int alarms_save(void) {
  assert(fs_save_file(ALARMS_FILENAME, (char *)&alarms, sizeof(alarms)) == 0);
  return 0;
}

int alarm_get(int idx, Alarm *dest_alarm) {
  if (idx < 0 || idx >= MAX_ALARMS) {
    return -1;
  }

  Alarm alarm = alarms.alarms[idx];
  if (alarm.name[0] == 0x00) {
    return -2;
  }

  if (dest_alarm != NULL) {
    *dest_alarm = alarm;
  }

  return 0;
}

int alarm_add(const struct Alarm *alarm) {
  for (int i = 0; i < MAX_ALARMS; i++) {
    if (alarm_get(i, NULL) == -2) {
      alarm_set(i, alarm);
      return i;
    }
  }
  return -1;
};

int alarm_set(int idx, const struct Alarm *src_alarm) {
  if (idx < 0 || idx >= MAX_ALARMS) {
    return -1;
  }

  if (src_alarm == NULL) {
    memset(&alarms.alarms[idx], 0, sizeof(alarms.alarms[0]));
    return 0;
  }

  memcpy(&alarms.alarms[idx], src_alarm, sizeof(Alarm));
  return 0;
}

int alarm_next_schedule(const Alarm *alarm, char today) {
  assert(alarm->days != 0x00);
  assert(today < 7);

  // TODO FIXME maybe this could be better.
  for (int i = 0; i < 7; i++) {
    // Find which days after today matches the alarm days, or something.
    if (alarm->days & (128 >> ((today + i) % 7))) {
      // In case if there is a match today originally, check if the schedule is
      // behind the current second.
      if (alarm->secondMark > (i * 24 * 60 * 60)) {
        continue;
      }

      return alarm->secondMark + (i * 24 * 60 * 60);
    }
  }

  // UNREACHABLE
  assert(false);
}

time_t alarm_earliest_alarm(const struct tm *now, Alarm *dest_alarm) {
  Alarm *earliest = NULL;
  int earliest_second = INT_MAX;
  for (int idx = 0; idx < MAX_ALARMS; idx++) {
    Alarm *test = &alarms.alarms[idx];
    if (test->name[0] == 0 || test->days == 0) {
      continue;
    }

    if (earliest == NULL) {
      earliest = test;
      earliest_second = alarm_next_schedule(earliest, now->tm_wday);
      continue;
    }

    int test_second = alarm_next_schedule(test, now->tm_wday);

    if (earliest_second > test_second) {
      earliest = test;
      earliest_second = test_second;
    }
  }

  if (earliest == NULL) {
    return -1;
  } else {
    // printmem("dest_alarm", dest_alarm, sizeof(dest_alarm));
    // printmem("earliest", earliest, sizeof(earliest));
    memcpy(dest_alarm, earliest, sizeof(Alarm));

    struct tm now_copy;
    memcpy(&now_copy, now, sizeof(tm));
    now_copy.tm_hour = 0;
    now_copy.tm_min = 0;
    now_copy.tm_sec = 0;

    time_t epoch = mktime(&now_copy);

    return epoch + earliest_second;
  }
}

int setup_alarm() {
  // if (!LittleFS.exists(ALARMS_FILENAME)) {
  // if (alarms_save() != 0) {
  //   return 1;
  // };
  //

  if (int err = alarms_load(); err == -2) {
    assert(fs_format());
    esp_restart();
  } else if (err != 0) {
    abort();
  };
  // };
  //

  strcpy(alarms.alarms[0].name, "first");
  alarms.alarms[0].days |= SUNDAY;
  alarms.alarms[0].secondMark = 5;

  strcpy(alarms.alarms[1].name, "second");
  alarms.alarms[1].days |= MONDAY;
  alarms.alarms[1].secondMark = 3;

  strcpy(alarms.alarms[2].name, "third");
  alarms.alarms[2].days |= MONDAY;

  struct tm now;
  now.tm_sec = 0;
  now.tm_min = 0;
  now.tm_hour = 0;
  now.tm_wday = 0;

  Alarm test = {};
  alarm_earliest_alarm(&now, &test);

  ESP_LOGD(ALARM_TAG, "earliest: %s", test.name);
  ESP_LOGD(ALARM_TAG, "%s: %d", alarms.alarms[0].name,
           alarm_next_schedule(&alarms.alarms[0], 0));
  ESP_LOGD(ALARM_TAG, "%s: %d", alarms.alarms[1].name,
           alarm_next_schedule(&alarms.alarms[1], 0));
  ESP_LOGD(ALARM_TAG, "%s: %d", alarms.alarms[2].name,
           alarm_next_schedule(&alarms.alarms[2], 0));

  return 0;
}
