#include "./ui.h"
#include <thirdparty/ST7789V.h>  // or Adafruit_ILI9341.h
#include <thirdparty/XPT2046.h>
#include "./menu/alarm.h"
#include "./menu/boot_logo.h"
#include "./menu/preferences.h"
#include "./pins.h"
#include "./webserver.h"
#include "./wifi.h"
#include "LittleFS.h"
#include "WiFi.h"
#include "clock.h"
#include "esp32-hal-gpio.h"
#include "menu/menu.h"
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

ST7789V tft = ST7789V(TFT_DC, TFT_CS);
XPT2046 ts(TOUCH_CS);
static TS_Calibration cal(TS_Point(13, 11), TS_Point(3530, 3465),
                          TS_Point(312, 113), TS_Point(381, 2275),
                          TS_Point(167, 214), TS_Point(2015, 710), SCREEN_WIDTH,
                          SCREEN_HEIGHT);

uint32_t ui_millis_cb(void);
void ui_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_buf);
void ui_touch_cb(lv_indev_t* indev, lv_indev_data_t* data);

int smc_init_drivers(void) {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PRI_BUTTON_PIN, INPUT_PULLDOWN);
  // pinMode(HAND_SENSOR_PIN, INPUT);
  // pinMode(SEC_BUTTON_PIN, INPUT_PULLDOWN);

  tft.init(320, 240);
  tft.fillScreen(0xFFFF);
  tft.drawImage(96, 56, 127, 127, (uint16_t*)BOOT_LOGO_SRC);

  assert(ts.begin());
  ts.calibrate(cal);

  lv_init();
  lv_tick_set_cb(ui_millis_cb);

  lv_display_t* display = lv_display_create(320, 240);

  /* LVGL will render to this 1/10 screen sized buffer for 2 bytes/pixel */
  static uint8_t buf[320 * 240 / 10 * 2];
  lv_display_set_buffers(display, buf, NULL, sizeof(buf),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  /* This callback will display the rendered image */
  lv_display_set_flush_cb(display, ui_flush_cb);

  lv_indev_t* indev = lv_indev_create();           /* Create input device */
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /* Set the device type */
  lv_indev_set_read_cb(indev, ui_touch_cb);        /* Set the read callback */

  test_menu();

  if (!LittleFS.begin()) {
    assert(LittleFS.format());
    esp_restart();
  }
  SMC_LOGI(TAG, "fs size: %ld/%ld\n", LittleFS.usedBytes(),
           LittleFS.totalBytes());

  preferences.setup();
  wifi.setup();
  rtc.setup();
  assert(sms.setup() == 0);
  alarms.setup();
  motor.setup();
  assert(preferences.save_into_fs() == 0);

  if (WiFi.status() == WL_CONNECTED) {
    assert(webserver.setup(&alarms) == 0);
  }

  struct tm now;
  Clock::get(&now);
  alarms.refresh(&now);

  SMC_LOGI(TAG, "alarm size: %d", sizeof(Alarm));
  SMC_LOGI(TAG, "alarm file size: %d", sizeof(Alarms));
  SMC_LOGI(TAG, "alarm log file size: %d", sizeof(AlarmLog));
  SMC_LOGI(TAG, "preferences file stroage size: %d", sizeof(DevicePreferences));
  SMC_LOGI(TAG, "wifi config size: %d", sizeof(WiFiConfig));

  SMC_LOGI(TAG, "took %ldms to boot", millis());

  return 0;
}

void smc_loop(void) {
  smc_internal_loop();
  lv_timer_handler();
  wifi.reconnect_loop();
  alarms.loop();
  static int last_compartment;
  if (alarms.should_move() != last_compartment) {
    smc_motor_move(alarms.should_move());
    last_compartment = alarms.should_move();
  }
  motor.loop();
}

int smc_motor_steps(void) {
  return motor.steps();
};

int smc_motor_compartment(void) {
  return motor.compartment();
};

void smc_motor_move(int compartment) {
  motor.spin_to(compartment);
};

bool smc_motor_running(void) {
  return motor.is_running();
};

struct Alarms* smc_system_alarms(void) {
  return &alarms;
}

int smc_sms_send(char* message, char* number);
int smc_sms_list(SMC_SMSMessage** dest, int max_len);
int smc_sms_signal(void);

int smc_wifi_add(struct SMC_WifiConfig* cfg);
int smc_wifi_remove(char* name);

int smc_wifi_connect(char* name) {
  assert(false);
};

int smc_wifi_disconnect(void) {
  return WiFi.disconnect(true, false);
};

int smc_wifi_signal(void) {
  return WiFi.RSSI();
};

int smc_wifi_scan(SMC_WifiConfig** dest, int max_len);
int smc_wifi_ap(bool state, char* pass);

void smc_alarm_buzzer_play(struct SMC_Melody) {
  digitalWrite(BUZZER_PIN, HIGH);
};

void smc_alarm_buzzer_off(void) {
  digitalWrite(BUZZER_PIN, LOW);
};

time_t smc_time_get(void) {
  struct tm now;
  rtc.get(&now);
  return mktime(&now);
};

int smc_battery_percentage(void) {
  return 50;
};

int smc_battery_powermode(void) {
  return 0;
};

void smc_battery_set_powermode(int mode) {
  assert(false);
};

int smc_preferences_load(void) {
  return preferences.load_from_fs();
};

int smc_preferences_save(void) {
  return preferences.save_into_fs();
};

int smc_fs_read(const char* path, void* dest, size_t len) {
  fs_mutex.lock();
  File file = LittleFS.open(path, FILE_READ);
  if (!file) {
    file.close();
    fs_mutex.unlock();
    SMC_LOGW(TAG, "no saved data at %s, ignoring", path);
    return -1;
  }
  assert(!file.isDirectory());

  size_t res = file.readBytes((char*)dest, len);
  if (res != len) {
    file.close();
    fs_mutex.unlock();
    SMC_LOGE(TAG, "reading from %s, res %d, size %d", path, res, len);
    return -2;
  };

  file.close();
  fs_mutex.unlock();

  return 0;
};

int smc_fs_write(const char* path, const void* src, size_t len) {
  fs_mutex.lock();
  File file = LittleFS.open(path, FILE_WRITE);

  assert(file && !file.isDirectory());

  assert(file.write((const uint8_t*)src, len) == len);

  SMC_LOGD(TAG, "write err is %d", file.getWriteError());
  file.close();
  fs_mutex.unlock();

  return 0;
};

void smc_data_reset(void) {
  assert(LittleFS.format());
};

void smc_device_restart(void) {
  esp_restart();
};

time_t smc_general_uptime(void) {
  return millis();
};

const char* smc_general_sw_info(void) {
  return "Mock Defense";
};

// TODO webapp interface and better names
int smc_webapp_local_toggle(bool state) {
  assert(false);
};

int smc_webapp_external_endpoints_toggle(bool state) {
  assert(false);
};

uint32_t ui_millis_cb(void) {
  return millis();
}

void ui_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_buf) {
  tft.drawImage(area->x1, area->y1, area->x2 - area->x1, area->y2 - area->y1,
                (uint16_t*)px_buf);
  lv_display_flush_ready(disp);
}

void ui_touch_cb(lv_indev_t* indev, lv_indev_data_t* data) {
  if (ts.touched()) {
    auto p = ts.getPoint();
    // Serial.printf("touched: %d, %d, %d\n", p.x, p.y, p.z);

    data->point.x = p.x;
    data->point.y = p.y;
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}
