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
  char name[51]; // 25+1 null and so on...
  char description[101];
  char category;
  char flags;
  char days;
  char icon;
  short color;    // just use a nice byte to rgb formula for this lol
  int secondMark; // Starting from GMT+0 Midnight
  time_t lastReminded;
  AlarmLog logs[5];
};

struct AlarmsFile {
  char version = ALARM_VERSION;
  Alarm alarms[MAX_ALARMS];
};

int setup_alarm(void);

int alarms_save(void);

int alarms_load(void);

// Returns the idx of the newly created alarm, negative return values are error
// codes. The newly created alarm will be placed in an index with invalid or
// empty alarm.
int alarm_add(const struct Alarm *src_alarm);

// Copies src_alarm to the dest alarm with the idx. If src_alarm is NULL, clears
// it instead.
int alarm_set(int idx, const struct Alarm *src_alarm);

// Copies src alarm with the idx to the dest_alarm. Returns -1 if there isn't
// any valid alarm with the idx.
int alarm_get(int idx, struct Alarm *dest_alarm);

// Appends a log to the alarm with the idx, if there is no room for new logs,
// clears the oldest log.
int alarm_append_log(int idx, const struct AlarmLog *log);

// Returns when the alarm will ring in seconds since today's midnight. This does
// not account for missed alarms!
int alarm_next_schedule(const struct Alarm *alarm, char today);

// Returns when an alarm will ring in seconds since the UNIX epoch, and sets
// dest_alarm to the earliest alarm. This does not account for missing alarms
// yet. Returns -1 if there are no valid alarms.
time_t alarm_earliest_alarm(const struct tm *now, Alarm *dest_alarm);

#endif
