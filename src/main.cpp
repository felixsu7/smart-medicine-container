#include "./alarm.h"
#include "./network.h"
#include "./pins.h"
#include "./preferences.h"
#include "./thirdparty/stepper.h"
#include "HardwareSerial.h"
#include "WiFi.h"
#include "esp32-hal-gpio.h"
#include "fs.h"
#include "uRTCLib.h"

static const char *MAIN_TAG = "main";

void must_succeed(int val) {
  if (val != 0) {
    ESP_LOGE(MAIN_TAG, "must_succeed got %d, looping", val);
    while (true) {
      Serial.print(".");
      delay(1000);
    };
  }
}

CheapStepper stepper(STEPPER_IN1, STEPPER_IN2, STEPPER_IN3, STEPPER_IN4);
uRTCLib rtc(0x68);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }
  Serial.setDebugOutput(true);

  esp_log_level_set("*", ESP_LOG_DEBUG);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PRI_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(SEC_BUTTON_PIN, INPUT_PULLDOWN);

  must_succeed(setup_fs());

  setup_wifi();
  // must_succeed(setup_time());
  URTCLIB_WIRE.begin();

  rtc.set(0, 42, 16, 6, 2, 5, 15);

  rtc.refresh();

  Serial.print("RTC DateTime: ");
  Serial.print(rtc.year());
  Serial.print('/');
  Serial.print(rtc.month());
  Serial.print('/');
  Serial.print(rtc.day());

  Serial.print(' ');

  Serial.print(rtc.hour());
  Serial.print(':');
  Serial.print(rtc.minute());
  Serial.print(':');
  Serial.print(rtc.second());

  Serial.print(" DOW: ");
  Serial.print(rtc.dayOfWeek());

  Serial.print(" - Temp: ");
  Serial.print(rtc.temp() / 100);

  Serial.println();

  must_succeed(setup_alarm());
  if (WiFi.status() == WL_CONNECTED) {
    setup_webserver();
  }

  // stepper.setRpm(16);
  //
  ESP_LOGI(MAIN_TAG, "alarm size: %d", sizeof(Alarm));
  ESP_LOGI(MAIN_TAG, "alarm file size: %d", sizeof(AlarmsFile));
  ESP_LOGI(MAIN_TAG, "preferences file stroage size: %d", sizeof(PreferencesFile));
  ESP_LOGI(MAIN_TAG, "wifi config size: %d", sizeof(WiFiConfig));

  ESP_LOGI(MAIN_TAG, "took %ldms to boot", millis());
}

long notifyCooldown;
int counter;

void loop() {
  wifi_reconnect_loop();

  // if (digitalRead(BUTTON_X) == HIGH) {
  //   stepper.step(true);
  // }
  //
  // if (digitalRead(BUTTON_Y) == HIGH) {
  //   stepper.step(false);
  // }
  //
  //
  if (digitalRead(SEC_BUTTON_PIN) == HIGH) {
    stepper.step(true);
  }

  if (digitalRead(PRI_BUTTON_PIN) == HIGH) {
    if (notifyCooldown + 5 * 1000 < millis()) {
      char message[100];
      sprintf(message, "Hello world! (%d)", counter++);

      ESP_LOGD(MAIN_TAG, "sending test notification");
      if (web_test_notify(message) == 200) {
        digitalWrite(LED_PIN, HIGH);
        delay(250);
        digitalWrite(LED_PIN, LOW);
      } else {
        digitalWrite(LED_PIN, HIGH);
        delay(1500);
        digitalWrite(LED_PIN, LOW);
      }
      notifyCooldown = millis();
    } else {
      ESP_LOGD(MAIN_TAG, "cooldown: %ldms left",
               (notifyCooldown + 5 * 1000) - millis());

      digitalWrite(LED_PIN, HIGH);
      delay(1000);
      digitalWrite(LED_PIN, LOW);
    }
  }
}
