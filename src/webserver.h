#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "PsychicHttpServer.h"
#include "menu/alarm.h"

class Webserver {
 public:
  int setup(Alarms* alarms);
  static int test_notify(const char* message);

 private:
  PsychicHttpServer server;
};

#endif
