#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "PsychicHttpServer.h"
#include "alarm.h"
#include "motor.h"
#include "thirdparty/ST7789v_arduino.h"

class Webserver {
 public:
  int setup(Alarms* alarms, Motor* motor, ST7789v_arduino* tft);
  static int test_notify(const char* message);

 private:
  PsychicHttpServer server;
};

#endif
