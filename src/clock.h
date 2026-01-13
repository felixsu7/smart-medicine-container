#ifndef CLOCK_H
#define CLOCK_H

#include <ctime>

int setup_clock(void);

// Syncs the local time from a NTP server, then also syncs the new time to the
// RTC Module.
int clock_sync_ntp(void);

int clock_get(struct tm *dest_tm);

time_t clock_get_epoch(void);

#endif
