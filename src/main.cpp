#include "./alarm.h"
#include "./network.h"
#include "./pins.h"
#include "./preferences.h"
#include "HardwareSerial.h"
#include "WiFi.h"
#include "clock.h"
#include "esp32-hal-gpio.h"
#include "fs.h"

static const char *MAIN_TAG = "main";

// CheapStepper stepper(STEPPER_IN1, STEPPER_IN2, STEPPER_IN3, STEPPER_IN4);

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

  if (int err = setup_fs(); err != 0) {
    // TODO FIXME
    ESP_LOGW(MAIN_TAG, "setup_fs got %d, formatting fs and restarting...", err);
    assert(fs_format() == 0);
    esp_restart();
  };

  setup_wifi();
  assert(setup_clock() == 0);

  if (int err = setup_alarm(); err != 0) {
    // TODO FIXME
    ESP_LOGW(MAIN_TAG, "setup_alarm got %d, formatting fs and restarting...",
             err);
    assert(fs_format() == 0);
    esp_restart();
  };

  if (WiFi.status() == WL_CONNECTED) {
    setup_webserver();
  }

  // stepper.setRpm(16);
  //
  ESP_LOGI(MAIN_TAG, "alarm size: %d", sizeof(Alarm));
  ESP_LOGI(MAIN_TAG, "alarm file size: %d", sizeof(AlarmsFile));
  ESP_LOGI(MAIN_TAG, "preferences file stroage size: %d",
           sizeof(PreferencesFile));
  ESP_LOGI(MAIN_TAG, "wifi config size: %d", sizeof(WiFiConfig));
  ESP_LOGI(MAIN_TAG, "days size: %d", sizeof(Days));

  ESP_LOGI(MAIN_TAG, "took %ldms to boot", millis());
}

long notifyCooldown;
int counter;

void loop() {
  wifi_reconnect_loop();

  // if (digitalRead(SEC_BUTTON_PIN) == HIGH) {
  //   stepper.step(true);
  // }

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
