#include "./alarm.h"
#include "./network.h"
#include "./pins.h"
#include "./preferences.h"
#include "HardwareSerial.h"
#include "WiFi.h"
#include "clock.h"
#include "esp32-hal-gpio.h"
#include "fs.h"

static const char *TAG = "main";

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

  assert(setup_fs() == 0);
  setup_wifi();
  assert(setup_clock() == 0);
  assert(setup_alarm() == 0);

  if (WiFi.status() == WL_CONNECTED) {
    assert(setup_webserver() == 0);
  }

  ESP_LOGI(TAG, "alarm size: %d", sizeof(Alarm));
  ESP_LOGI(TAG, "alarm file size: %d", sizeof(AlarmsFile));
  ESP_LOGI(TAG, "preferences file stroage size: %d", sizeof(PreferencesFile));
  ESP_LOGI(TAG, "wifi config size: %d", sizeof(WiFiConfig));
  ESP_LOGI(TAG, "days size: %d", sizeof(Days));

  ESP_LOGI(TAG, "took %ldms to boot", millis());
}

long notifyCooldown;
int counter;

void loop() {
  wifi_reconnect_loop();

  if (digitalRead(PRI_BUTTON_PIN) == HIGH) {
    if (notifyCooldown + 5 * 1000 < millis()) {
      char message[100];
      sprintf(message, "Hello world! (%d)", counter++);

      ESP_LOGD(TAG, "sending test notification");

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
      ESP_LOGD(TAG, "cooldown: %ldms left",
               (notifyCooldown + 5 * 1000) - millis());

      digitalWrite(LED_PIN, HIGH);
      delay(1000);
      digitalWrite(LED_PIN, LOW);
    }
  }
}
