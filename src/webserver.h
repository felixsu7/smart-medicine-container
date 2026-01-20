#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "PsychicHttpServer.h"
#include "alarm.h"
#include "motor.h"

class Webserver {
 public:
  int setup(Alarms* alarms, Motor* motor);
  static int test_notify(const char* message);

 private:
  PsychicHttpServer server;
};

#endif
