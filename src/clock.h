#ifndef CLOCK_H
#define CLOCK_H

#include <ctime>

int setup_clock(void);

// Fetches the updated time from a set of NTP servers, and sets the system's
// time and the RTC module to it. Returns -1 if it cannot fetch the time.
int clock_sync_ntp(void);

// Helper function that also checks for correctness.
int clock_get(struct tm *now);

#endif
