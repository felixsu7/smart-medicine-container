#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "PsychicHttpServer.h"
#include "alarm.h"
#include "motor.h"
#include "sms.h"
#include "thirdparty/ST7789V.h"

class Webserver {
 public:
  int setup(Alarms* alarms, Motor* motor, ST7789V* tft, SMS* sms);
  static int test_notify(const char* message);

 private:
  PsychicHttpServer server;
};

#endif
