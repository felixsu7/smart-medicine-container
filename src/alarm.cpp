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

int alarm_file_load(const char *filename) {
  if (fs_load_file(filename, (char *)&alarms, sizeof(alarms)) != 0) {
    return 1;
  }

  if (alarms.magic != ALARM_MAGIC) {
    ESP_LOGE(ALARM_TAG, "magic mismatch: %04X", alarms.magic);
    return 2;
  }

  if (alarms.version != ALARM_VERSION) {
    ESP_LOGE(ALARM_TAG, "unsupported version: %02X", alarms.version);
    return 3;
  }

  return 0;
}

int alarm_file_save(const char *filename) {
  if (fs_save_file(filename, (char *)&alarms, sizeof(alarms)) != 0) {
    return 1;
  }
  return 0;
}

// TODO: could be improved with bitwise ops, maybe
bool index_day(days target, int idx) {
  switch (idx % 7) {
  case 0:
    return target.sunday;
  case 1:
    return target.monday;
  case 2:
    return target.tuesday;
  case 3:
    return target.wednesday;
  case 4:
    return target.thursday;
  case 5:
    return target.friday;
  case 6:
    return target.saturday;
  default:
    return false;
  }
}

bool is_zero(days target) {
  // TODO
  if (target.amountInstead) {
    return true;
  }
  for (int i = 0; i < 7; i++) {
    if (index_day(target, i)) {
      return false;
    }
  }

  return true;
}

long alarm_next_schedule(const Alarm *alarm, const struct tm *now) {
  if (is_zero(alarm->days)) {
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

int alarm_earliest_alarm(const struct tm *now, Alarm *dest_alarm) {
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
  if (alarm_file_save(ALARMS_FILENAME) != 0) {
    return 1;
  };
  //
  if (alarm_file_load(ALARMS_FILENAME) != 0) {
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
