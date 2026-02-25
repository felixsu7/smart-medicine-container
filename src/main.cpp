#include <./ui.h>
#include <Arduino.h>

static const char* TAG = "main";

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  Serial.setDebugOutput(true);
  esp_log_level_set("*", ESP_LOG_DEBUG);

  smc_init_drivers();
}

void loop() {
  smc_loop();

  // static long hand_sensor_tk;
  // if (bounce(&hand_sensor_tk) && analogRead(HAND_SENSOR_PIN) < 500) {
  //   char msg[100];
  //   memset(msg, 0, sizeof(msg));
  //   // TODO define calibration values and make it a system of sorts
  //   snprintf(msg, sizeof(msg), "HS Val: %d", analogRead(HAND_SENSOR_PIN));
  //   Serial.println(msg);
  // }

  // TODO only affect the lvgl timers
  // delay(5);

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

  // if (alarms.is_ringing() > -1) {
  //   digitalWrite(BUZZER_PIN, HIGH);
  // } else {
  //   digitalWrite(BUZZER_PIN, LOW);
  // }

  // static long touched_tk;
  // if (bounce(&touched_tk, 10) && ts.touched()) {
  //   TS_Point p = ts.getPoint();
  //   tft.drawPixel(p.x, p.y, random());
  // }
}
