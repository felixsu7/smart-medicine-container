#ifndef WEB_H
#define WEB_H

#include "alarm.h"

// TODO: seperate wifi from web

int setup_wifi(void);
int setup_webserver(Alarms *alarms);

int wifi_reconnect_loop(void);
int web_test_notify(const char *message);

#endif
