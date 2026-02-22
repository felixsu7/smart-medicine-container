#include <SPI.h>
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
#include "thirdparty/lvgl/lvgl.h"
#include "utils.h"

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

uint32_t lvglMillis(void) {
  return millis();
}

void my_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_buf) {
  Serial.printf("begin flush 1");
  /* Show the rendered image on the display */
  tft.startWrite();
  Serial.printf("begin flush 2");
  tft.writeAddrWindow(area->x1, area->y1, area->x2, area->y2);
  Serial.printf("begin flush 3");
  int px_buf_size =
      (area->x2 - area->x1) * (area->y2 - area->y1) * 2;  // 16bit now
  tft.write(px_buf, px_buf_size);

  Serial.printf("begin flush 4");
  tft.endWrite();

  Serial.printf("flush: %d", px_buf_size);

  lv_display_flush_ready(disp);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  Serial.setDebugOutput(true);

  esp_log_level_set("*", ESP_LOG_DEBUG);

  tft.init(320, 240);
  tft.fillScreen(COLOR_BLACK);

  lv_init();
  lv_tick_set_cb(lvglMillis);

  lv_display_t* display = lv_display_create(320, 240);

  /* LVGL will render to this 1/10 screen sized buffer for 2 bytes/pixel */
  static uint8_t buf[320 * 240 / 10 * 2];
  lv_display_set_buffers(display, buf, NULL, sizeof(buf),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  /* This callback will display the rendered image */
  lv_display_set_flush_cb(display, my_flush_cb);

  /* Create widgets */
  lv_obj_t* label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "Hello LVGL!");

  // tft.fillScreen(COLOR_ROSE);
  // mu_Context* ctx = &renderer.muCtx;
  // mu_begin(ctx);
  // if (mu_begin_window(ctx, "My Window", mu_rect(10, 10, 140, 86))) {
  //   int temp[] = {60, -1};
  //   mu_layout_row(ctx, 2, temp, 0);
  //
  //   mu_label(ctx, "First:");
  //   if (mu_button(ctx, "Button1")) {
  //     printf("Button1 pressed\n");
  //   }
  //
  //   mu_label(ctx, "Second:");
  //   if (mu_button(ctx, "Button2")) {
  //     mu_open_popup(ctx, "My Popup");
  //   }
  //
  //   if (mu_begin_popup(ctx, "My Popup")) {
  //     mu_label(ctx, "Hello world!");
  //     mu_end_popup(ctx);
  //   }
  //
  //   mu_end_window(ctx);
  // }
  // mu_end(ctx);
  // renderer.present(false);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PRI_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(HAND_SENSOR_PIN, INPUT);
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
  lv_timer_handler();
  wifi.reconnect_loop();
  alarms.loop();
  motor.loop();

  static long hand_sensor_tk;
  if (bounce(&hand_sensor_tk)) {
    char msg[100];
    memset(msg, 0, sizeof(msg));
    // TODO define calibration values and make it a system of sorts
    snprintf(msg, sizeof(msg), "HS Val: %d", analogRead(HAND_SENSOR_PIN));
    Serial.println(msg);
  }

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

  // static long touched_tk;
  // if (bounce(&touched_tk, 10) && ts.touched()) {
  //   TS_Point p = ts.getPoint();
  //   tft.drawPixel(p.x, p.y, random());
  // }
}
