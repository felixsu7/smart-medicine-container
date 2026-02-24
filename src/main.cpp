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

static uint16_t const SCREEN_WIDTH = 320;
static uint16_t const SCREEN_HEIGHT = 240;

enum class PointID { NONE = -1, A, B, C, COUNT };

static TS_Calibration cal(TS_Point(13, 11), TS_Point(3530, 3465),
                          TS_Point(312, 113), TS_Point(381, 2275),
                          TS_Point(167, 214), TS_Point(2015, 710), SCREEN_WIDTH,
                          SCREEN_HEIGHT);

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
  tft.drawImage(area->x1, area->y1, area->x2 - area->x1, area->y2 - area->y1,
                (uint16_t*)px_buf);
  lv_display_flush_ready(disp);
}

void lvglTouch(lv_indev_t* indev, lv_indev_data_t* data) {
  if (ts.touched()) {
    auto p = ts.getPoint();
    Serial.printf("touched: %d, %d, %d\n", p.x, p.y, p.z);

    data->point.x = p.x;
    data->point.y = p.y;
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

typedef enum {
  LV_MENU_ITEM_BUILDER_VARIANT_1,
  LV_MENU_ITEM_BUILDER_VARIANT_2
} lv_menu_builder_variant_t;

static void back_event_handler(lv_event_t* e);
static void switch_handler(lv_event_t* e);
static lv_obj_t* root_page;
static lv_obj_t* create_text(lv_obj_t* parent, const char* icon,
                             const char* txt,
                             lv_menu_builder_variant_t builder_variant);
static lv_obj_t* create_slider(lv_obj_t* parent, const char* icon,
                               const char* txt, int32_t min, int32_t max,
                               int32_t val);
static lv_obj_t* create_switch(lv_obj_t* parent, const char* icon,
                               const char* txt, bool chk);

static void back_event_handler(lv_event_t* e) {
  lv_obj_t* obj = lv_event_get_target_obj(e);
  lv_obj_t* menu = (lv_obj_t*)lv_event_get_user_data(e);

  if (lv_menu_back_button_is_root(menu, obj)) {
    lv_obj_t* mbox1 = lv_msgbox_create(NULL);
    lv_msgbox_add_title(mbox1, "Hello");
    lv_msgbox_add_text(mbox1, "Root back btn click.");
    lv_msgbox_add_close_button(mbox1);
  }
}

static void switch_handler(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* menu = (lv_obj_t*)lv_event_get_user_data(e);
  lv_obj_t* obj = lv_event_get_target_obj(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
      lv_menu_set_page(menu, NULL);
      lv_menu_set_sidebar_page(menu, root_page);
      lv_obj_send_event(
          lv_obj_get_child(
              lv_obj_get_child(lv_menu_get_cur_sidebar_page(menu), 0), 0),
          LV_EVENT_CLICKED, NULL);
    } else {
      lv_menu_set_sidebar_page(menu, NULL);
      lv_menu_clear_history(
          menu); /* Clear history because we will be showing the root page later */
      lv_menu_set_page(menu, root_page);
    }
  }
}

static lv_obj_t* create_text(lv_obj_t* parent, const char* icon,
                             const char* txt,
                             lv_menu_builder_variant_t builder_variant) {
  lv_obj_t* obj = lv_menu_cont_create(parent);

  lv_obj_t* img = NULL;
  lv_obj_t* label = NULL;

  if (icon) {
    img = lv_image_create(obj);
    lv_image_set_src(img, icon);
  }

  if (txt) {
    label = lv_label_create(obj);
    lv_label_set_text(label, txt);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_obj_set_flex_grow(label, 1);
  }

  if (builder_variant == LV_MENU_ITEM_BUILDER_VARIANT_2 && icon && txt) {
    lv_obj_add_flag(img, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    lv_obj_swap(img, label);
  }

  return obj;
}

static lv_obj_t* create_slider(lv_obj_t* parent, const char* icon,
                               const char* txt, int32_t min, int32_t max,
                               int32_t val) {
  lv_obj_t* obj =
      create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_2);

  lv_obj_t* slider = lv_slider_create(obj);
  lv_obj_set_flex_grow(slider, 1);
  lv_slider_set_range(slider, min, max);
  lv_slider_set_value(slider, val, LV_ANIM_OFF);

  if (icon == NULL) {
    lv_obj_add_flag(slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  }

  return obj;
}

static lv_obj_t* create_switch(lv_obj_t* parent, const char* icon,
                               const char* txt, bool chk) {
  lv_obj_t* obj =
      create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_1);

  lv_obj_t* sw = lv_switch_create(obj);
  lv_obj_add_state(sw, chk ? LV_STATE_CHECKED : LV_STATE_DEFAULT);

  return obj;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  Serial.setDebugOutput(true);

  esp_log_level_set("*", ESP_LOG_DEBUG);

  tft.init(320, 240);
  tft.fillScreen(0xFFFF);

  assert(ts.begin());
  ts.calibrate(cal);

  lv_init();
  lv_tick_set_cb(lvglMillis);

  lv_display_t* display = lv_display_create(320, 240);

  /* LVGL will render to this 1/10 screen sized buffer for 2 bytes/pixel */
  static uint8_t buf[320 * 240 / 10 * 2];
  lv_display_set_buffers(display, buf, NULL, sizeof(buf),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  /* This callback will display the rendered image */
  lv_display_set_flush_cb(display, my_flush_cb);

  lv_indev_t* indev = lv_indev_create();           /* Create input device */
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /* Set the device type */
  lv_indev_set_read_cb(indev, lvglTouch);          /* Set the read callback */

  // test menu

  lv_obj_t* menu = lv_menu_create(lv_screen_active());

  lv_color_t bg_color = lv_obj_get_style_bg_color(menu, LV_PART_MAIN);
  if (lv_color_brightness(bg_color) > 127) {
    lv_obj_set_style_bg_color(
        menu,
        lv_color_darken(lv_obj_get_style_bg_color(menu, LV_PART_MAIN), 10), 0);
  } else {
    lv_obj_set_style_bg_color(
        menu,
        lv_color_darken(lv_obj_get_style_bg_color(menu, LV_PART_MAIN), 50), 0);
  }
  lv_menu_set_mode_root_back_button(menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);
  lv_obj_add_event_cb(menu, back_event_handler, LV_EVENT_CLICKED, menu);
  lv_obj_set_size(menu, lv_display_get_horizontal_resolution(NULL),
                  lv_display_get_vertical_resolution(NULL));
  lv_obj_center(menu);

  Serial.println("test");

  lv_obj_t* cont;
  lv_obj_t* section;

  /*Create sub pages*/
  lv_obj_t* sub_mechanics_page = lv_menu_page_create(menu, NULL);
  lv_obj_set_style_pad_hor(
      sub_mechanics_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  lv_menu_separator_create(sub_mechanics_page);
  section = lv_menu_section_create(sub_mechanics_page);
  create_slider(section, LV_SYMBOL_SETTINGS, "Velocity", 0, 150, 120);
  create_slider(section, LV_SYMBOL_SETTINGS, "Acceleration", 0, 150, 50);
  create_slider(section, LV_SYMBOL_SETTINGS, "Weight limit", 0, 150, 80);

  Serial.println("test");

  lv_obj_t* sub_sound_page = lv_menu_page_create(menu, NULL);
  lv_obj_set_style_pad_hor(
      sub_sound_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  lv_menu_separator_create(sub_sound_page);
  section = lv_menu_section_create(sub_sound_page);
  create_switch(section, LV_SYMBOL_AUDIO, "Sound", false);

  lv_obj_t* sub_display_page = lv_menu_page_create(menu, NULL);
  lv_obj_set_style_pad_hor(
      sub_display_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  lv_menu_separator_create(sub_display_page);
  section = lv_menu_section_create(sub_display_page);
  create_slider(section, LV_SYMBOL_SETTINGS, "Brightness", 0, 150, 100);

  lv_obj_t* sub_software_info_page = lv_menu_page_create(menu, NULL);
  lv_obj_set_style_pad_hor(
      sub_software_info_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  section = lv_menu_section_create(sub_software_info_page);
  create_text(section, NULL, "Version 1.0", LV_MENU_ITEM_BUILDER_VARIANT_1);

  Serial.println("test");

  lv_obj_t* sub_legal_info_page = lv_menu_page_create(menu, NULL);
  lv_obj_set_style_pad_hor(
      sub_legal_info_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  section = lv_menu_section_create(sub_legal_info_page);
  for (uint32_t i = 0; i < 15; i++) {
    create_text(section, NULL,
                "This is a long long long long long long long long long text, "
                "if it is long enough it may scroll.",
                LV_MENU_ITEM_BUILDER_VARIANT_1);
  }

  Serial.println("test");

  lv_obj_t* sub_about_page = lv_menu_page_create(menu, NULL);
  lv_obj_set_style_pad_hor(
      sub_about_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  lv_menu_separator_create(sub_about_page);
  section = lv_menu_section_create(sub_about_page);
  cont = create_text(section, NULL, "Software information",
                     LV_MENU_ITEM_BUILDER_VARIANT_1);
  lv_menu_set_load_page_event(menu, cont, sub_software_info_page);
  cont = create_text(section, NULL, "Legal information",
                     LV_MENU_ITEM_BUILDER_VARIANT_1);
  lv_menu_set_load_page_event(menu, cont, sub_legal_info_page);

  Serial.println("test");

  lv_obj_t* sub_menu_mode_page = lv_menu_page_create(menu, NULL);
  lv_obj_set_style_pad_hor(
      sub_menu_mode_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  lv_menu_separator_create(sub_menu_mode_page);
  section = lv_menu_section_create(sub_menu_mode_page);
  cont = create_switch(section, LV_SYMBOL_AUDIO, "Sidebar enable", true);
  lv_obj_add_event_cb(lv_obj_get_child(cont, 2), switch_handler,
                      LV_EVENT_VALUE_CHANGED, menu);

  Serial.println("test");

  /*Create a root page*/
  root_page = lv_menu_page_create(menu, "Settings");
  lv_obj_set_style_pad_hor(
      root_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  section = lv_menu_section_create(root_page);
  cont = create_text(section, LV_SYMBOL_SETTINGS, "Mechanics",
                     LV_MENU_ITEM_BUILDER_VARIANT_1);
  lv_menu_set_load_page_event(menu, cont, sub_mechanics_page);
  cont = create_text(section, LV_SYMBOL_AUDIO, "Sound",
                     LV_MENU_ITEM_BUILDER_VARIANT_1);
  lv_menu_set_load_page_event(menu, cont, sub_sound_page);
  cont = create_text(section, LV_SYMBOL_SETTINGS, "Display",
                     LV_MENU_ITEM_BUILDER_VARIANT_1);
  lv_menu_set_load_page_event(menu, cont, sub_display_page);

  create_text(root_page, NULL, "Others", LV_MENU_ITEM_BUILDER_VARIANT_1);
  section = lv_menu_section_create(root_page);
  cont = create_text(section, NULL, "About", LV_MENU_ITEM_BUILDER_VARIANT_1);
  lv_menu_set_load_page_event(menu, cont, sub_about_page);
  cont = create_text(section, LV_SYMBOL_SETTINGS, "Menu mode",
                     LV_MENU_ITEM_BUILDER_VARIANT_1);
  lv_menu_set_load_page_event(menu, cont, sub_menu_mode_page);

  lv_menu_set_sidebar_page(menu, root_page);

  Serial.println("test");
  lv_obj_send_event(
      lv_obj_get_child(lv_obj_get_child(lv_menu_get_cur_sidebar_page(menu), 0),
                       0),
      LV_EVENT_CLICKED, NULL);

  //
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
  if (bounce(&hand_sensor_tk) && analogRead(HAND_SENSOR_PIN) < 500) {
    char msg[100];
    memset(msg, 0, sizeof(msg));
    // TODO define calibration values and make it a system of sorts
    snprintf(msg, sizeof(msg), "HS Val: %d", analogRead(HAND_SENSOR_PIN));
    Serial.println(msg);
  }

  delay(5);

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
