#include "./alarm.h"
#include "./web.h"
#include "WiFi.h"
#include "esp32-hal-gpio.h"
#include "fs.h"

static const char *MAIN_TAG = "main";

#define LED 26
#define BUTTON 25
#define BUTTON_X 34
#define BUTTON_Y 35

void must_succeed(int val) {
  if (val != 0) {
    ESP_LOGE(MAIN_TAG, "must_succeed got %d, looping", val);
    while (true) {
      Serial.print(".");
      delay(1000);
    };
  }
}

// CheapStepper stepper(27, 26, 25, 33);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }
  Serial.setDebugOutput(true);

  esp_log_level_set("*", ESP_LOG_DEBUG);

  pinMode(LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLDOWN);

  // pinMode(BUTTON_X, INPUT_PULLDOWN);
  // pinMode(BUTTON_Y, INPUT_PULLDOWN);

  must_succeed(setup_fs());

  setup_wifi();
  // must_succeed(setup_time());
  must_succeed(setup_alarms());
  if (WiFi.status() == WL_CONNECTED) {
    setup_webserver();
  }

  // stepper.setRpm(16);

  ESP_LOGI(MAIN_TAG, "took %ldms to boot", millis());
}

long notifyCooldown;
int counter;

void loop() {
  reconnect_loop();

  // if (digitalRead(BUTTON_X) == HIGH) {
  //   stepper.step(true);
  // }
  //
  // if (digitalRead(BUTTON_Y) == HIGH) {
  //   stepper.step(false);
  // }
  //

  if (digitalRead(BUTTON) == HIGH) {
    if (notifyCooldown + 5 * 1000 < millis()) {
      char message[100];
      sprintf(message, "Hello world! (%d)", counter++);

      ESP_LOGD(MAIN_TAG, "sending test notification");
      if (testNotify(message) == 200) {
        digitalWrite(LED, HIGH);
        delay(250);
        digitalWrite(LED, LOW);
      } else {
        digitalWrite(LED, HIGH);
        delay(1500);
        digitalWrite(LED, LOW);
      }
      notifyCooldown = millis();
    } else {
      ESP_LOGD(MAIN_TAG, "cooldown: %ldms left",
               (notifyCooldown + 5 * 1000) - millis());

      digitalWrite(LED, HIGH);
      delay(1000);
      digitalWrite(LED, LOW);
    }
  }
}
