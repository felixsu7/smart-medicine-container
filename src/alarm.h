#ifndef ALARM_H
#define ALARM_H

#include "./config.h"
#include <ctime>

static const char ALARM_VERSION = 0x00;

enum Days {
  AMOUNT_INSTEAD = 128,
  SUNDAY = 64,
  MONDAY = 32,
  TUESDAY = 16,
  WEDNESDAY = 8,
  THURSDAY = 4,
  FRIDAY = 2,
  SATURDAY = 1,
};

struct AlarmLog {
  time_t when;
  char flags;
};

struct Alarm {
  char name[50];
  char description[100];
  char category;
  char flags;
  char days; // See the Days Enum.
  char icon;
  short color;         // Just use a nice formula for this.
  int secondMark;      // Starting from GMT+0 Midnight
  time_t lastReminded; // When the user has last attended this alarm, for
                       // detecting missed alarms.
  AlarmLog logs[5];
};

struct AlarmsFile {
  char version = ALARM_VERSION;
  Alarm alarms[MAX_ALARMS];
};

int setup_alarm(void);

int alarms_save(void);

int alarms_load(void);

// Adds a copy of alarm into the storage and returns the index of the added
// alarm. Returns -1 if the storage is full. The added alarm will be at an index
// where there is an empty or invalid alarm.
int alarm_add(const struct Alarm *alarm);

// Copies alarm to the storage on the specified index. Returns -1 if the
// index is out of bounds. If alarm is NULL, clears it instead.
int alarm_set(int idx, const struct Alarm *alarm);

// Copies the alarm from storage with specified index to alarm. Returns
// Returns -1 if the index is out of bounds. Returns -2 if the index is invalid
// or empty. If alarm is NULL, no copying is done, just checks for
// validity.
int alarm_get(int idx, struct Alarm *alarm);

// Adds the log into the alarm in storage with specified index. If there is no
// room for new logs, clears the oldest log and place the new log there instead.
// Returns -1 if the index is out of bounds.
int alarm_log(int idx, const struct AlarmLog *log);

// Returns when the alarm will ring in seconds since today's midnight. This does
// not account for missed alarms. Returns -1 if today is out of bounds. Returns
// -2 if alarm data is invalid.
int alarm_next_schedule(const struct Alarm *alarm, char today);

// Returns the seconds since the UNIX timestamp of when the alarm will ring
// based on seconds since today's midnight. Returns -1 if alarm is invalid.
time_t alarm_epoch(const struct Alarm *alarm, const struct tm *now, int secs);

// Returns when any alarm in the storage will ring in seconds since the UNIX
// epoch, and copies that alarm to the alarm. This does not account for missing
// alarms yet. If alarm is NULL, no copying will be done. Returns -1 if there
// are no valid alarms.
time_t alarm_earliest_alarm(const struct tm *now, Alarm *alarm);

#endif
