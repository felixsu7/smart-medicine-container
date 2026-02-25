#ifndef ALARM_H
#define ALARM_H

#include <ctime>
#include "./config.h"

static const char ALARM_VERSION = 0x00;

enum DAYS_MASK {
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
  char name[51];
  char description[101];
  char compartment;
  char category;
  char flags;
  char days;  // See the Days Enum.
  char icon;
  short color;          // Just use a nice formula for this.
  int secondMark;       // Starting from GMT+0 Midnight
  time_t lastReminded;  // When the user has last attended this alarm, for
                        // detecting missed alarms.
  AlarmLog logs[5];
};

class Alarms {
 public:
  int setup(void);

  int load_from_fs(void);
  int save_into_fs(void);

  // Call in the loop to monitor ringing alarms.
  void loop(void);

  // Reevaluates the earliest alarm to be monitored. Returns -1 on error.
  // Returns 1 if refreshing is disabled.
  int refresh(const struct tm* now);

  // Marks an alarm at index as ringing. There can be only one alarm ringing
  // currently (TODO). See Alarms::attend() to clear it.
  int ring(int idx);

  // Checks when there is an alarm ringing, if there is, return the index of
  // that alarm. Returns -1 if there is none. TODO weird return values?
  int is_ringing(void);

  // Attends the currently ringing alarm. Returns 0 on success, -1 if there is
  // none ringing. flags is currently unused.
  int attend(time_t when, char flags);

  // Ring without an associated alarm at a schedule, overrides all alarms and
  // disables refreshes.
  int one_off_ring(time_t when);

  // Returns when an alarm would ring, the index of said alarm would be set on
  // the idx_ptr, can be NULL. If set to -2,  there is no associated alarm
  // (one-off). If set to -1, no alarm. Other negative values are errors.
  time_t ring_in(int* idx_ptr);

  // Copies the alarm from storage with specified index to alarm. Returns
  // Returns -1 if the index is out of bounds. Returns -2 if the index is
  // invalid or empty. If alarm is NULL, no copying is done, just checks for
  // validity.
  int get(int idx, struct Alarm* alarm);

  // Copies alarm to the storage on the specified index. If alarm is NULL,
  // clears it instead. If index is -1, checks for validty for the alarm instead
  // of setting. Returns -1 if the index is out of bounds. Returns -2 if both
  // the index is -1 and alarm is NULL. Returns -3 if alarm is invalid.
  int set(int idx, const struct Alarm* alarm);

  // Adds a copy of alarm into the storage and returns the index of the added
  // alarm. Returns -1 if the storage is full. The added alarm will be at an
  // index where there is an empty or invalid alarm.
  int add(const struct Alarm* alarm);

  // Adds the log into the alarm in the storage with specified index. If there
  // is no room for new logs, clears the oldest log and place the new log there
  // instead. Returns -1 if the index is out of bounds. Returns 0 if the log was
  // appended without clearing the oldest log, 1 otherwise.
  int append_log(int idx, const struct AlarmLog* log);

  // TODO Marks an alarm in the storage at index.
  int attend_idx(int idx, time_t when, char flags);

  // Returns when any alarm in the storage will ring in seconds since the UNIX
  // epoch, and copies that alarm to the alarm. This does not account for
  // missing alarms yet. If alarm is NULL, no copying will be done. Returns -1
  // if there are no valid alarms.
  time_t earliest_alarm(const struct tm* now, Alarm* alarm, int* idx_ptr);

  // Returns when the alarm will ring in seconds based on today (0-6) and secs
  // since GMT+0 midnight. This does not account for missed alarms. Returns -1
  // if today is out of bounds. Returns -2 if alarm data is invalid.
  static int next_schedule(const struct Alarm* alarm, char today, int sec);

  // Returns the index of the compartment that the steppper motor should move to.
  int should_move(void);

  // Returns the seconds since the UNIX timestamp of when the alarm will ring
  // based on seconds since today's midnight. Returns -1 if alarm is invalid.
  static time_t epoch(const struct Alarm* alarm, const struct tm* now,
                      int secs);

  char version = ALARM_VERSION;
  Alarm list[MAX_ALARMS];

  // When to ring the alarm at earliest_idx in UNIX timestamp GMT+0.
  time_t when_ring;
  // The index of the earliest alarm that would ring; see Alarms::refresh(). If
  // it is -1, the alarm ringing mechanism is disabled.
  int earliest_idx = -1;
  // Currently ringing alarm. -1 means theres none. Alarms::ring() should not
  // generally replace this. TODO: maybe an array would be better?
  int ringing_idx = -1;
  // Currently, first flag (1) is if earliest_idx doesn't point to an alarm
  // (one-off). Second flag (2) is if refreshing should be disabled, usually
  // combined with the first flag.
  char ringing_flags;
  // Last ringed alarm's compartment.
  char last_compartment;
};

#endif
