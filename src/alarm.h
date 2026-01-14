#ifndef ALARM_H
#define ALARM_H

#include "./config.h"
#include <ctime>

static const uint16_t ALARM_MAGIC = 0x5412;
static const char ALARM_VERSION = 0x00;

struct AlarmLog {
  time_t when;
  char flags;
};

struct Days {
  bool amountInstead : 1; // UNUSED for now
  bool sunday : 1;
  bool monday : 1;
  bool tuesday : 1;
  bool wednesday : 1;
  bool thursday : 1;
  bool friday : 1;
  bool saturday : 1;
};

char days_char(struct Days days);
struct Days char_days(char days);

struct Alarm {
  char name[51]; // 25+1 null and so on...
  char description[101];
  char category;
  char flags;
  struct Days days;
  char icon;
  short color;    // just use a nice byte to rgb formula for this lol
  int secondMark; // Starting from GMT+0 Midnight
  time_t lastReminded;
  AlarmLog logs[5];
};

struct AlarmsFile {
  short magic = ALARM_MAGIC;
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

// Copies src_alarm to the dest alarm with the idx.
int alarm_set(int idx, const struct Alarm *src_alarm);

// Copies src alarm with the idx to the dest_alarm. Returns -1 if there isn't
// any valid alarm with the idx.
int alarm_get(int idx, struct Alarm *dest_alarm);

// Clears the alarm with the idx, the idx should be treated as an errorous
// reference afterwards.
int alarm_remove(int idx);

// Appends a log to the alarm with the idx, if there is no room for new logs,
// clears the oldest log.
int alarm_append_log(int idx, const struct AlarmLog *log);

// Returns when the alarm will ring, the timestamp is in seconds starting from
// the UNIX epoch. This does not account for missed alarms!
time_t alarm_next_schedule(const struct Alarm *alarm, const struct tm *now);

// Returns when an alarm will ring, same return value as alarm_next__schedule,
// and sets dest_alarm to the earliest alarm. This does not account for missing
// alarms yet.
time_t alarm_earliest_alarm(const struct tm *now, Alarm *dest_alarm);

#endif
