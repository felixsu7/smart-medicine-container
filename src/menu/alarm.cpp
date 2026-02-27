#include "./alarm.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ctime>
#include "../ui.h"
#include "config.h"
#include "time.h"

static const char* TAG = "alarm";

int Alarms::load_from_fs(void) {
  int code = smc_fs_read(ALARMS_PATH, this, sizeof(Alarms));

  // fs_mutex.lock();
  // File file = LittleFS.open(ALARMS_PATH, FILE_READ);
  // if (!file) {
  //   file.close();
  //   fs_mutex.unlock();
  //   ESP_LOGW(TAG, "no saved data at %s, ignoring", ALARMS_PATH);
  //   return -1;
  // }
  // assert(!file.isDirectory());
  //
  // size_t res = file.readBytes((char*)this, sizeof(Alarms));
  // if (res != sizeof(Alarms)) {
  //   file.close();
  //   fs_mutex.unlock();
  //   SMC_LOGE(TAG, "reading from %s, res %d, size %d", ALARMS_PATH, res,
  //            sizeof(Alarms));
  //   return -2;
  // };
  //
  // file.close();
  // fs_mutex.unlock();

  // TODO DEBUG
  //  char dump[256 * 3 + 1];
  // int dump_res = hexdump(dump, this, sizeof(Alarms));
  // SMC_LOGD(TAG, "first %d bytes dump of %s: %s", 256, ALARMS_PATH, dump);
  //
  // if (version != ALARM_VERSION) {
  //   SMC_LOGE(TAG, "unsupported version: %02X", version);
  //   return -3;
  // }

  return code;
}

int Alarms::save_into_fs(void) {
  return smc_fs_write(ALARMS_PATH, this, sizeof(Alarms));
  // fs_mutex.lock();
  // File file = LittleFS.open(ALARMS_PATH, FILE_WRITE);
  //
  // assert(file && !file.isDirectory());
  //
  // assert(file.write((const uint8_t*)this, sizeof(Alarms)) == sizeof(Alarms));
  // assert(file.write(version) == 1);
  //
  // for (int i = 0; i < MAX_ALARMS; i++) {
  //   assert(file.write((uint8_t *)&list[i], sizeof(Alarm)) == sizeof(Alarm));
  // }
  //
  // SMC_LOGD(TAG, "write err is %d", file.getWriteError());
  // file.close();
  // fs_mutex.unlock();

  // return 0;
}

void Alarms::loop(void) {
  if (earliest_idx == -1) {
    return;
  }

  if (time(NULL) > when_ring) {
    // SMC_LOGD(TAG, "%ld, %ld", time(NULL), when_ring);
    ring(earliest_idx);
  }
}

int Alarms::ring(int idx) {
  if (ringing_idx != -1) {
    // There is another alarm ringing.
    return -1;
  }

  ringing_idx = idx;

  return 0;
}

int Alarms::attend(time_t when, char flags) {
  if (ringing_flags & 1) {
    ringing_flags = !ringing_flags;
    ringing_flags |= 3;
    ringing_flags = !ringing_flags;
    earliest_idx = -1;
    ringing_idx = -1;
    when_ring = -1;
  } else {
    if (earliest_idx == -1) {
      return -1;
    }

    assert(attend_idx(earliest_idx, when, flags) == 0);
  }
  // TODO
  struct tm now;
  gmtime_r(&when, &now);

  refresh(&now);

  return 0;
}

int Alarms::attend_idx(int idx, time_t when, char flags) {
  if (idx < 0 || idx >= MAX_ALARMS) {
    return -1;
  }

  last_compartment = list[idx].compartment;

  list[idx].lastReminded = when;
  AlarmLog log;
  log.when = when;
  log.flags = 0x00;

  int err = append_log(idx, &log);
  // SMC_LOGD(TAG, "append_log err is %d", err);
  assert(err >= 0);

  earliest_idx = -1;
  ringing_idx = -1;
  when_ring = -1;

  return 0;
}

int Alarms::is_ringing(void) {
  if (ringing_idx >= 0) {
    return ringing_idx;
  }
  return -1;
}

int Alarms::refresh(const struct tm* now) {
  if (ringing_flags & 2) {
    return 1;
  }

  int idx;
  time_t when = earliest_alarm(now, NULL, &idx);
  // SMC_LOGD(TAG, "when: %ld", when);
  if (when < 0) {
    return -1;
  }

  earliest_idx = idx;
  when_ring = when;

  return 0;
}

time_t Alarms::ring_in(int* idx_ptr) {
  if (when_ring == 0) {
    if (idx_ptr != NULL) {
      *idx_ptr = -1;
    }
    return 0;
  }

  if (idx_ptr != NULL) {
    if (earliest_idx == -1) {
      // One-off
      *idx_ptr = -2;
      return when_ring;
    }
    *idx_ptr = earliest_idx;
  }

  return when_ring;
}

int Alarms::one_off_ring(time_t when) {
  if (ringing_idx != -1) {
    return -1;
  }
  earliest_idx = 0;
  when_ring = when;
  ringing_flags = 3;
  return 0;
}

int Alarms::add(const struct Alarm* alarm) {
  // FIXME?
  int err = set(-1, alarm);
  // ESP_LOGW(TAG, "err add is %d", err);
  if (err == -3) {
    return -2;
  }

  for (int i = 0; i < MAX_ALARMS; i++) {
    if (get(i, NULL) == -2) {
      set(i, alarm);
      return i;
    }
  }
  return -1;
};

int Alarms::set(int idx, const struct Alarm* alarm) {
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

int Alarms::get(int idx, struct Alarm* alarm) {
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

int Alarms::append_log(int idx, const struct AlarmLog* log) {
  if (idx < 0 || idx >= MAX_ALARMS) {
    return -1;
  }

  time_t oldest_when = 0;
  int oldest_idx;

  // TODO amount of logs (5) is currently hardcoded.
  for (int i = 0; i < 5; i++) {
    if (list[idx].logs[i].when == 0) {
      memcpy(&list[idx].logs[i], log, sizeof(AlarmLog));
      return 0;
    }

    // SMC_LOGD(TAG, "idx %d when %ld", i, list[idx].logs[i].when);

    if (oldest_when < list[idx].logs[i].when) {
      oldest_when = list[idx].logs[i].when;
      oldest_idx = i;
    }
  }

  memcpy(&list[idx].logs[oldest_idx], log, sizeof(AlarmLog));

  return 1;
}

int Alarms::next_schedule(const struct Alarm* alarm, char today,
                          int today_sec) {
  if (today > 7) {
    return -1;
  }
  if ((alarm->days & 127) == 0x00) {
    return -2;
  }

  // SMC_LOGD(TAG, "today %d", today);
  // SMC_LOGD(TAG, "days %d", alarm->days);

  // TODO FIXME maybe this could be better.
  for (int i = 0; i < 8; i++) {
    // SMC_LOGD(TAG, "i %d", i);
    // Find which days after today matches the alarm days, or something.
    if (alarm->days & (SUNDAY >> ((today + i) % 7))) {
      // SMC_LOGD(TAG, "true 1");
      // In case if there is a match today originally, check if the schedule is
      // behind the current second.
      if (i == 0 && alarm->secondMark < today_sec) {
        // SMC_LOGD(TAG, "true 2");
        continue;
      }

      return alarm->secondMark + (i * 24 * 60 * 60) - today_sec;
    }

    // SMC_LOGD(TAG, "false 1");
  }

  // UNREACHABLE
  assert(false);
}

time_t Alarms::earliest_alarm(const struct tm* now, struct Alarm* alarm,
                              int* idx_ptr) {
  if (now == NULL) {
    return -2;
  }

  Alarm* earliest = NULL;
  int earliest_second = 0xFFFFFFFF;
  int earliest_idx = -1;
  int today_sec = (now->tm_hour * 60 * 60) + (now->tm_min * 60) + now->tm_sec;

  for (int i = 0; i < MAX_ALARMS; i++) {
    Alarm test = list[i];
    if (set(-1, &test)) {
      continue;
    }

    if (earliest == NULL) {
      earliest = &test;
      earliest_second = next_schedule(earliest, now->tm_wday, today_sec);
      earliest_idx = i;
      continue;
    }

    int test_second = next_schedule(&test, now->tm_wday, today_sec);

    if (earliest_second > test_second) {
      earliest = &test;
      earliest_second = test_second;
      earliest_idx = i;
    }
  }

  if (earliest == NULL) {
    return -1;
  } else {
    // printmem("dest_alarm", dest_alarm, sizeof(dest_alarm));
    // printmem("earliest", earliest, sizeof(earliest));
    if (alarm != NULL) {
      memcpy(alarm, earliest, sizeof(Alarm));
    }

    if (idx_ptr != NULL) {
      *idx_ptr = earliest_idx;
    }

    struct tm now_copy;
    memcpy(&now_copy, now, sizeof(tm));

    time_t epoch = mktime(&now_copy);

    return epoch + earliest_second;
  }
}

int Alarms::should_move(void) {
  return last_compartment;
}

int Alarms::setup(void) {
  // TODO DEBUG
  if (int err = load_from_fs(); err == -2) {
    smc_data_reset();
    smc_device_restart();
  } else if (err < -2) {
    // SMC_LOGE(TAG, "err is %d", err);
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
  // SMC_LOGD(TAG, "earliest: %s", test.name);
  // SMC_LOGD(TAG, "%s: %d", list[0].name, next_schedule(&list[0], 0,
  // today_sec)); SMC_LOGD(TAG, "%s: %d", list[1].name, next_schedule(&list[1],
  // 0, today_sec)); SMC_LOGD(TAG, "%s: %d", list[2].name,
  // next_schedule(&list[2], 0, today_sec));
  //
  return 0;
}
