#ifndef CLOCK_H
#define CLOCK_H

int setup_clock();
int clock_sync_ntp();
int clock_get_local(struct tm *dest_tm);

long clock_get_epoch();

#endif
