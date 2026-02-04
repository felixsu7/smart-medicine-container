#ifndef SMS_H
#define SMS_H

#include <HardwareSerial.h>

class SMS {
 public:
  int setup(void);
  uint16_t IO(const char* send_msg, char* recv_buf, uint16_t max_recv,
              int ms = 1000);

 private:
  HardwareSerial simSerial = HardwareSerial(2);
};

#endif
