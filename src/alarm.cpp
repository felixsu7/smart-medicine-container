#include "./alarm.h"
#include "./utils.h"
#include "config.h"
#include "esp32-hal-log.h"
#include "time.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fs.h>

static const char *ALARM_TAG = "alarm";

static AlarmsFile alarms;

char days_char(struct Days days) { return *(char *)&days; }

struct Days char_days(char days) { return *(struct Days *)&days; }

int alarms_load(void) {
  int err = fs_load_file(ALARMS_FILENAME, (char *)&alarms, sizeof(alarms));

  if (err == 1) {
    ESP_LOGI(ALARM_TAG, "creating new alarms file...");
    if (alarms_save() != 0) {
      return 1;
    }
  }

  if (err == 2) {
    ESP_LOGW(ALARM_TAG, "alarm struct size is different from alarm file");
    return 2;
  }

  if (alarms.magic != ALARM_MAGIC) {
    ESP_LOGE(ALARM_TAG, "magic mismatch: %04X", alarms.magic);
    return 3;
  }

  if (alarms.version != ALARM_VERSION) {
    ESP_LOGE(ALARM_TAG, "unsupported version: %02X", alarms.version);
    return 4;
  }

  return 0;
}

int alarms_save(void) {
  if (fs_save_file(ALARMS_FILENAME, (char *)&alarms, sizeof(alarms)) != 0) {
    return 1;
  }
  return 0;
}

bool index_day(char days, int idx) {
  char mask = 1 << (idx % 7);
  return days && mask;
}

bool index_day(struct Days days, int idx) {
  char mask = 1 << (idx % 7);
  return days_char(days) && mask;
}

int alarm_get(int idx, Alarm *dest_alarm) {
  if (idx < 0 || idx >= MAX_ALARMS) {
    return -2;
  }

  Alarm alarm = alarms.alarms[idx];
  if (alarm.name[0] == 0x00) {
    return -1;
  }

  if (dest_alarm != NULL) {
    *dest_alarm = alarm;
  }
  return 0;
}

int alarm_add(const struct Alarm *alarm) {
  for (int i = 0; i < MAX_ALARMS; i++) {
    if (alarm_get(i, NULL) == -1) {
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
  memcpy(&alarms.alarms[idx], src_alarm, sizeof(Alarm));
  return 0;
}

time_t alarm_next_schedule(const Alarm *alarm, const struct tm *now) {
  if (days_char(alarm->days) == 0x00) {
    return -1;
  }
  int today = now->tm_wday;
  int days = 0;
  int seconds = alarm->secondMark -
                ((now->tm_hour * (60 * 60)) + (now->tm_min * 60) + now->tm_sec);

  // ESP_LOGD(ALARM_TAG, "adad %d %ld %d %d %d", today, seconds, now->tm_hour,
  // now->tm_min, now->tm_sec);

  while (true) {
    // ESP_LOGD(ALARM_TAG, "days %d", days);
    if (days > 14) {
      ESP_LOGE(ALARM_TAG, "days > 14");
      return -1;
    }

    if (index_day(alarm->days, today + days)) {
      break;
    }
    days++;
  }
  seconds += days * 24 * 60 * 60;

  return seconds;
}

time_t alarm_earliest_alarm(const struct tm *now, Alarm *dest_alarm) {
  Alarm *earliest = NULL;
  for (int idx = 0; idx < MAX_ALARMS; idx++) {
    // ESP_LOGD(ALARM_TAG, "idx %d", idx);
    Alarm *temp = &alarms.alarms[idx];
    if (temp->name[0] == 0x00) {
      break;
    }

    if (earliest == NULL) {
      earliest = temp;
      continue;
    }

    if (alarm_next_schedule(temp, now) < alarm_next_schedule(earliest, now)) {
      earliest = temp;
    }
  }

  if (earliest == NULL) {
    return 1;
  } else {
    printmem("dest_alarm", dest_alarm, sizeof(dest_alarm));
    printmem("earliest", earliest, sizeof(earliest));
    memcpy(dest_alarm, earliest, sizeof(Alarm));

    return 0;
  }
}

int setup_alarm() {
  // if (!LittleFS.exists(ALARMS_FILENAME)) {
  // if (alarms_save() != 0) {
  //   return 1;
  // };
  //
  if (alarms_load() != 0) {
    return 1;
  };
  // };
  //

  strcpy(alarms.alarms[0].name, "first");
  alarms.alarms[0].days.sunday = true;
  alarms.alarms[0].secondMark = 5;

  strcpy(alarms.alarms[1].name, "second");
  alarms.alarms[1].days.monday = true;
  alarms.alarms[1].secondMark = 3;

  strcpy(alarms.alarms[2].name, "third");
  alarms.alarms[2].days.monday = true;

  struct tm now;
  now.tm_sec = 0;
  now.tm_min = 0;
  now.tm_hour = 0;
  now.tm_wday = 0;

  Alarm test = {};
  alarm_earliest_alarm(&now, &test);

  ESP_LOGD(ALARM_TAG, "earliest: %s", test.name);
  ESP_LOGD(ALARM_TAG, "%s: %d", alarms.alarms[0].name,
           alarm_next_schedule(&alarms.alarms[0], &now));
  ESP_LOGD(ALARM_TAG, "%s: %d", alarms.alarms[1].name,
           alarm_next_schedule(&alarms.alarms[1], &now));
  ESP_LOGD(ALARM_TAG, "%s: %d", alarms.alarms[2].name,
           alarm_next_schedule(&alarms.alarms[2], &now));

  return 0;
}
