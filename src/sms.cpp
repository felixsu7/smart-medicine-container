#include <pins.h>
#include <sms.h>

static const char* TAG = "sms";

// Helper function to easily perform IO (Input/Output) Operations with a serial connection in one function. Good for transmitting a message and expecting to recieve messages as well.
uint16_t serialIO(HardwareSerial serial, const char* send_msg, char* recv_buf,
                  uint16_t max_recv, int ms = 1000) {
  if (!serial) {
    return 0;
  }

  if (send_msg != NULL) {
    serial.println(send_msg);
  }

  uint16_t recv = 0;

  if (recv_buf != NULL) {
    delay(ms);
    memset(recv_buf, 0, max_recv);
    while (serial.available() && recv < max_recv - 1) {
      recv_buf[recv++] = serial.read();
    }
    recv_buf[recv] = 0x00;
  }

  return recv;
}

int SMS::setup(void) {
  simSerial.begin(9600, SERIAL_8N1, SIM_RX, SIM_TX);
  char buf[10];
  serialIO(simSerial, "AT", buf, sizeof(buf), 1000);
  ESP_LOGD(TAG, "got sim resp: %s", buf);
  return 0;
}

uint16_t SMS::IO(const char* send_msg, char* recv_buf, uint16_t max_recv,
                 int ms) {
  return serialIO(simSerial, send_msg, recv_buf, max_recv, ms);
}
