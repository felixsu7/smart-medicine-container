#ifndef ALARM_H
#define ALARM_H

#include "./config.h"
#include <ctime>

static const short ALARMS_FILE_MAGIC = 0x5412;
static const char ALARMS_FILE_VERSION = 0x01;

struct AlarmLog {
  time_t when;
  char flags;
};

struct days {
  bool amountInstead : 1; // UNUSED for now
  bool sunday : 1;
  bool monday : 1;
  bool tuesday : 1;
  bool wednesday : 1;
  bool thursday : 1;
  bool friday : 1;
  bool saturday : 1;
};

struct Alarm {
  char name[51]; // 25+1 null and so on...
  char description[101];
  char category;
  char flags;
  struct days days;
  char icon;
  short color;    // just use a nice byte to rgb formula for this lol
  int secondMark; // Starting from GMT+0 Midnight
  time_t lastReminded;
  AlarmLog logs;
};

struct AlarmStorage {
  short magic = ALARMS_FILE_MAGIC;
  char version = ALARMS_FILE_VERSION;
  Alarm storage[MAX_ALARMS];
};

time_t next_schedule(Alarm *alarm, struct tm *now);
Alarm *earliest_alarm(struct tm *now);
int setup_alarms();

#endif
