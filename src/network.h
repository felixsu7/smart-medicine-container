#ifndef WEB_H
#define WEB_H

#include "PsychicHttpServer.h"
#include "WiFiMulti.h"
#include "alarm.h"

class Wifi {
public:
  int setup(void);
  static int reconnect_loop(void);

private:
  WiFiMulti wifi_multi;
};

class Webserver {
public:
  int setup(Alarms *alarms);
  static int test_notify(const char *message);

private:
  PsychicHttpServer server;
};

#endif
