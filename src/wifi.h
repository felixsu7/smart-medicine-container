#ifndef WIFI_H
#define WIFI_H

#include "WiFiMulti.h"

class Wifi {
 public:
  int setup(void);
  static int reconnect_loop(void);

 private:
  WiFiMulti wifi_multi;
};

#endif
