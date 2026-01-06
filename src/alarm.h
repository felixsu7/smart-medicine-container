#ifndef ALARM_H
#define ALARM_H

#include "./config.h"
#include <ctime>

static const short ALARM_MAGIC = 0x5412;
static const char ALARM_VERSION = 0x00;

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
  char name[50]; // 25+1 null and so on...
  char description[100];
  char category;
  char flags;
  struct days days;
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

int setup_alarm();

int alarm_file_save(const char *filename);
int alarm_file_load(const char *filename);

int alarm_add(const char *name, const struct days day, int secondMark);
int alarm_get(char id, Alarm *dest_alarm);
int alarm_remove(char id);

int alarm_set_name(char id, const char *name);
int alarm_set_desc(char id, const char *desc);
int alarm_set_category(char id, char category);
int alarm_set_days(char id, bool sun, bool mon, bool tue, bool wed, bool thu,
                   bool fri, bool sat);
int alarm_set_days_amount(char id, int amount);
int alarm_set_icon(char id, char icon);
int alarm_set_color(char id, short color);
int alarm_set_seconds(char id, int secondMark);
int alarm_set_last_reminded(char id, int last_reminded);
int alarm_append_log(char id, const struct AlarmLog *log);
int alarm_get_logs(char id, struct AlarmLog **dest_logs);
int alarm_clear_logs(char id);

long alarm_next_schedule(const Alarm *alarm, const struct tm *now);
int alarm_earliest_alarm(const struct tm *now, Alarm *dest_alarm);
int alarm_sorted_list(Alarm **dest_alarms);

#endif
