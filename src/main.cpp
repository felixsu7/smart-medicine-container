#include <SPI.h>
#include <pins.h>
#include <thirdparty/ST7789V.h>  // or Adafruit_ILI9341.h
#include <thirdparty/XPT2046.h>
#include "./alarm.h"
#include "./pins.h"
#include "./preferences.h"
#include "./webserver.h"
#include "./wifi.h"
#include "HardwareSerial.h"
#include "LittleFS.h"
#include "WiFi.h"
#include "clock.h"
#include "esp32-hal-gpio.h"
#include "motor.h"
#include "sms.h"
#include "utils.h"

static uint16_t const SCREEN_WIDTH = 320;
static uint16_t const SCREEN_HEIGHT = 240;

enum class PointID { NONE = -1, A, B, C, COUNT };

static TS_Calibration cal(TS_Point(13, 11), TS_Point(3530, 3465),
                          TS_Point(312, 113), TS_Point(381, 2275),
                          TS_Point(167, 214), TS_Point(2015, 710), SCREEN_WIDTH,
                          SCREEN_HEIGHT);

static const char* TAG = "main";

DevicePreferences preferences;
Wifi wifi;
Clock rtc;
Alarms alarms;
Webserver webserver;
Motor motor;
SMS sms;

XPT2046 ts(TOUCH_CS);
ST7789V tft = ST7789V(TFT_DC, TFT_CS);

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  Serial.setDebugOutput(true);

  esp_log_level_set("*", ESP_LOG_DEBUG);

  tft.init(320, 240);
  tft.fillScreen(COLOR_DARKGRAY);

  assert(ts.begin());
  ts.calibrate(cal);

  // pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PRI_BUTTON_PIN, INPUT_PULLDOWN);
  // pinMode(SEC_BUTTON_PIN, INPUT_PULLDOWN);

  if (!LittleFS.begin()) {
    assert(LittleFS.format());
    esp_restart();
  }
  ESP_LOGI(TAG, "fs size: %ld/%ld\n", LittleFS.usedBytes(),
           LittleFS.totalBytes());

  preferences.setup();
  wifi.setup();
  rtc.setup();
  assert(sms.setup() == 0);
  alarms.setup();
  motor.setup();
  assert(preferences.save_into_fs() == 0);

  if (WiFi.status() == WL_CONNECTED) {
    assert(webserver.setup(&alarms, &motor, &tft, &sms) == 0);
  }

  struct tm now;
  Clock::get(&now);
  alarms.refresh(&now);

  ESP_LOGI(TAG, "alarm size: %d", sizeof(Alarm));
  ESP_LOGI(TAG, "alarm file size: %d", sizeof(Alarms));
  ESP_LOGI(TAG, "alarm log file size: %d", sizeof(AlarmLog));
  ESP_LOGI(TAG, "preferences file stroage size: %d", sizeof(DevicePreferences));
  ESP_LOGI(TAG, "wifi config size: %d", sizeof(WiFiConfig));

  ESP_LOGI(TAG, "took %ldms to boot", millis());
}

void loop() {
  wifi.reconnect_loop();
  alarms.loop();
  motor.loop();

  // static long ring_forecast_tk;
  // if (bounce(&ring_forecast_tk)) {
  //   int idx;
  //   time_t when_ring = alarms.ring_in(&idx);
  //   if (when_ring > 0) {
  //     ESP_LOGD(TAG, "idx %d ringing in %lds", idx, when_ring - time(NULL));
  //   }
  // }

  // static long current_ringing_tk;
  // if (digitalRead(SEC_BUTTON_PIN) == HIGH) {
  //   if (bounce(&current_ringing_tk, 200, true)) {
  //     if (alarms.is_ringing() == -1) {
  //       ESP_LOGI(TAG, "no currently ringing alarm");
  //     } else {
  //       alarms.attend(time(NULL), 0x00, &motor);
  //     }
  //   }
  // }

  // static long test_notify_tk;
  // // if (digitalRead(PRI_BUTTON_PIN) == HIGH) {
  //   if (bounce(&test_notify_tk, 5000, true)) {
  //     char message[100];
  //     static int counter;
  //     sprintf(message, "Hello world! (%d)", counter++);
  //
  //     ESP_LOGD(TAG, "sending test notification");
  //
  //     if (webserver.test_notify(message) == 200) {
  //       digitalWrite(LED_PIN, HIGH);
  //       delay(250);
  //       digitalWrite(LED_PIN, LOW);
  //     } else {
  //       digitalWrite(LED_PIN, HIGH);
  //       delay(1500);
  //       digitalWrite(LED_PIN, LOW);
  //     }
  //   }
  // }

  if (alarms.is_ringing() > -1) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  static long touched_tk;
  if (bounce(&touched_tk, 10) && ts.touched()) {
    TS_Point p = ts.getPoint();
    tft.drawPixel(p.x, p.y, random());
  }
}
