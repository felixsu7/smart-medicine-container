#include "./alarm.h"
#include "LittleFS.h"
#include "config.h"
#include "esp32-hal-log.h"
#include "time.h"
#include "utils.h"
#include <Esp.h>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ctime>

static const char *TAG = "alarm";

int Alarms::load_from_fs(void) {
  char data[sizeof(list) + 1];
  memset(data, 0, sizeof(data));

  File file = LittleFS.open(ALARMS_PATH, FILE_READ);
  assert(!file.isDirectory());
  if (!file) {
    ESP_LOGW(TAG, "no saved data at %s, ignoring", ALARMS_PATH);
    return -1;
  }

  size_t res = file.readBytes(data, sizeof(data));
  if (res != sizeof(data)) {
    ESP_LOGE(TAG, "reading from %s, res %d, size %d", ALARMS_PATH, res,
             sizeof(data));
    return -2;
  };

  file.close();

  // TODO DEBUG
  char dump[256 * 3 + 1];
  int dump_res = hexdump(dump, data, sizeof(data));
  ESP_LOGD(TAG, "first %d bytes dump of %s: %s", 256, ALARMS_PATH, dump);

  if (version != ALARM_VERSION) {
    ESP_LOGE(TAG, "unsupported version: %02X", version);
    return -3;
  }

  return 0;
}

int Alarms::save_into_fs(void) {
  File file = LittleFS.open(ALARMS_PATH, FILE_WRITE);

  assert(file && !file.isDirectory());
  assert(file.write(version) == 1);

  for (int i = 0; i < MAX_ALARMS; i++) {
    assert(file.write((uint8_t *)&list[i], sizeof(Alarm)) == sizeof(Alarm));
  }

  file.flush();
  file.close();

  return 0;
}

int Alarms::add(const struct Alarm *alarm) {
  // FIXME?
  int err = set(-1, alarm);
  if (err == -3) {
    return -2;
  }

  for (int i = 0; i < MAX_ALARMS; i++) {
    if (get(i, NULL) == -2) {
      return i;
    }
  }
  return -1;
};

int Alarms::set(int idx, const struct Alarm *alarm) {
  // FIXME not returning -3 if the alarm invalid
  //
  if (idx < -1 || idx >= MAX_ALARMS) {
    return -1;
  }

  if (idx == -1 && alarm == NULL) {
    return -2;
  }

  if (alarm == NULL) {
    memset(&list[idx], 0, sizeof(list[0]));
    return 0;
  }

  if (alarm->name[0] == 0x00 || (alarm->days & 127) == 0x00) {
    return -3;
  }

  if (idx == -1) {
    return 0;
  }

  memcpy(&list[idx], alarm, sizeof(Alarm));
  return 0;
}

int Alarms::get(int idx, struct Alarm *alarm) {
  if (idx < 0 || idx >= MAX_ALARMS) {
    return -1;
  }

  Alarm a = list[idx];
  if (set(-1, &a) == -3) {
    return -2;
  }

  if (alarm != NULL) {
    memcpy(alarm, &a, sizeof(Alarm));
  }

  return 0;
}

int Alarms::next_schedule(const struct Alarm *alarm, char today,
                          int today_sec) {
  if (today > 7) {
    return -1;
  }
  if ((alarm->days & 127) == 0x00) {
    return -2;
  }

  // ESP_LOGD(TAG, "today %d", today);
  // ESP_LOGD(TAG, "days %d", alarm->days);

  // TODO FIXME maybe this could be better.
  for (int i = 0; i < 8; i++) {
    // ESP_LOGD(TAG, "i %d", i);
    // Find which days after today matches the alarm days, or something.
    if (alarm->days & (SUNDAY >> ((today + i) % 7))) {
      // ESP_LOGD(TAG, "true 1");
      // In case if there is a match today originally, check if the schedule is
      // behind the current second.
      if (i == 0 && alarm->secondMark < today_sec) {
        // ESP_LOGD(TAG, "true 2");
        continue;
      }

      return alarm->secondMark + (i * 24 * 60 * 60) - today_sec;
    }

    // ESP_LOGD(TAG, "false 1");
  }

  // UNREACHABLE
  assert(false);
}

time_t Alarms::earliest_alarm(const struct tm *now, struct Alarm *alarm) {
  Alarm *earliest = NULL;
  int earliest_second = INT_MAX;
  int today_sec = (now->tm_hour * 60 * 60) + (now->tm_min * 60) + now->tm_sec;

  for (int idx = 0; idx < MAX_ALARMS; idx++) {
    Alarm test = list[idx];
    if (set(-1, &test)) {
      continue;
    }

    if (earliest == NULL) {
      earliest = &test;
      earliest_second = next_schedule(earliest, now->tm_wday, today_sec);
      continue;
    }

    int test_second = next_schedule(&test, now->tm_wday, today_sec);

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

    struct tm now_copy;
    memcpy(&now_copy, now, sizeof(tm));

    time_t epoch = mktime(&now_copy);

    return epoch + earliest_second;
  }
}

int Alarms::setup(void) {
  // TODO DEBUG
  if (int err = load_from_fs(); err == -2) {
    assert(LittleFS.format());
    esp_restart();
  } else if (err < -2) {
    ESP_LOGE(TAG, "err is %d", err);
    assert(false);
  };

  // strcpy(list[0].name, "first");
  // list[0].days |= SUNDAY;
  // list[0].secondMark = 5;
  //
  // strcpy(list[1].name, "second");
  // list[1].days |= MONDAY;
  // list[1].secondMark = 3;
  //
  // strcpy(list[2].name, "third");
  // list[2].days |= MONDAY;
  //
  // struct tm now;
  // now.tm_sec = 0;
  // now.tm_min = 0;
  // now.tm_hour = 0;
  // now.tm_wday = 0;
  //
  // Alarm test = {};
  // earliest_alarm(&now, &test);
  //
  // int today_sec = (now.tm_hour * 60 * 60) + (now.tm_min * 60) + now.tm_sec;
  //
  // ESP_LOGD(TAG, "earliest: %s", test.name);
  // ESP_LOGD(TAG, "%s: %d", list[0].name, next_schedule(&list[0], 0,
  // today_sec)); ESP_LOGD(TAG, "%s: %d", list[1].name, next_schedule(&list[1],
  // 0, today_sec)); ESP_LOGD(TAG, "%s: %d", list[2].name,
  // next_schedule(&list[2], 0, today_sec));
  //
  return 0;
}
