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
#include "thirdparty/ST7789v_arduino.h"
#include "utils.h"

static const char* TAG = "main";

DevicePreferences preferences;
Wifi wifi;
Clock rtc;
Alarms alarms;
Webserver webserver;
Motor motor;

#define TFT_DC 16
#define TFT_RST -1
#define TFT_CS 17    // only for displays with CS pin
#define TFT_MOSI 23  // for hardware SPI data pin (all of available pins)
#define TFT_SCLK 18  // for hardware SPI sclk pin (all of available pins)
ST7789v_arduino tft =
    ST7789v_arduino(TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_CS);

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  Serial.setDebugOutput(true);

  esp_log_level_set("*", ESP_LOG_DEBUG);

  tft.init(320, 240);
  tft.fillScreen(RED);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PRI_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(SEC_BUTTON_PIN, INPUT_PULLDOWN);

  if (!LittleFS.begin()) {
    assert(LittleFS.format());
    esp_restart();
  }
  ESP_LOGI(TAG, "fs size: %ld/%ld\n", LittleFS.usedBytes(),
           LittleFS.totalBytes());

  preferences.setup();
  wifi.setup();
  rtc.setup();
  alarms.setup();
  motor.setup();
  // assert(preferences.save_into_fs() == 0);

  if (WiFi.status() == WL_CONNECTED) {
    assert(webserver.setup(&alarms, &motor) == 0);
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

  static long current_ringing_tk;
  if (digitalRead(SEC_BUTTON_PIN) == HIGH) {
    if (bounce(&current_ringing_tk, 200, true)) {
      if (alarms.is_ringing() == -1) {
        ESP_LOGI(TAG, "no currently ringing alarm");
      } else {
        alarms.attend(time(NULL), 0x00, &motor);
      }
    }
  }

  // static long touch_irq_tk;
  // if (bounce(&touch_irq_tk)) {
  //  ESP_LOGD(TAG, "touch irq pin: %d", analogRead(4));
  // }

  static long test_notify_tk;
  if (digitalRead(PRI_BUTTON_PIN) == HIGH) {
    if (bounce(&test_notify_tk, 5000, true)) {
      char message[100];
      static int counter;
      sprintf(message, "Hello world! (%d)", counter++);

      ESP_LOGD(TAG, "sending test notification");

      if (webserver.test_notify(message) == 200) {
        digitalWrite(LED_PIN, HIGH);
        delay(250);
        digitalWrite(LED_PIN, LOW);
      } else {
        digitalWrite(LED_PIN, HIGH);
        delay(1500);
        digitalWrite(LED_PIN, LOW);
      }
    }
  }

  if (alarms.is_ringing() > -1) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}
