#ifndef CLOCK_H
#define CLOCK_H

#include <thirdparty/uRTCLib.h>
#include <ctime>

class Clock {
 public:
  int setup(void);

  // Fetches the updated time from a set of NTP servers, and sets the system's
  // time and the RTC module to it. Returns -1 if it cannot fetch the time.
  static int sync_ntp(void);

  // Returns time in GMT+0, also checks for correctness.
  static int get(struct tm* now);

 private:
  uRTCLib rtc;
};
#endif
