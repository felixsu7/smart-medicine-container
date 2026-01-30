#include <SPI.h>
#include <pins.h>
#include <thirdparty/ST7789v_arduino.h>  // or Adafruit_ILI9341.h
#include <thirdparty/XPT2046_Calibrated.h>
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
#include "thirdparty/XPT2046_Calibrated.h"
#include "utils.h"

// -- CONFIGURATION --

static uint16_t const SCREEN_WIDTH = 320;
static uint16_t const SCREEN_HEIGHT = 240;

enum class PointID { NONE = -1, A, B, C, COUNT };

// source points used for calibration
static TS_Point _screenPoint[] = {
    TS_Point(13, 11),    // point A
    TS_Point(312, 113),  // point B
    TS_Point(167, 214)   // point C
};

// touchscreen points used for calibration verification
static TS_Point _touchPoint[] = {
    TS_Point(3795, 3735),  // point A
    TS_Point(482, 2200),   // point B
    TS_Point(2084, 583),   // point C
};

static TS_Calibration cal(_screenPoint[(int)PointID::A],
                          _touchPoint[(int)PointID::A],
                          _screenPoint[(int)PointID::B],
                          _touchPoint[(int)PointID::B],
                          _screenPoint[(int)PointID::C],
                          _touchPoint[(int)PointID::C], SCREEN_WIDTH,
                          SCREEN_HEIGHT);

static const char* TAG = "main";

DevicePreferences preferences;
Wifi wifi;
Clock rtc;
Alarms alarms;
Webserver webserver;
Motor motor;

XPT2046_Calibrated ts(TOUCH_CS);

ST7789v_arduino tft = ST7789v_arduino(TFT_DC, -1, TFT_CS);

// -----------------------------------------------------------------------------

// -- UTILITY ROUTINES (DECLARATION) --

inline bool touched();
void crosshair(TS_Point p);
uint16_t distance(TS_Point a, TS_Point b);
void drawMapping(PointID n, TS_Point p);
void updateScreenEdges(TS_Point p);
PointID nearestScreenPoint(TS_Point touch);

// -- ARDUINO MAIN --

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  // assert(Wire.begin());
  Serial.setDebugOutput(true);

  esp_log_level_set("*", ESP_LOG_DEBUG);

  // I2C Address Scanner
  // int devices = 0;
  // for (int addr = 1; addr < 127; addr++) {
  //   char msg[20];
  //   sprintf(msg, "scanning %02X", addr);
  //   Serial.println(msg);
  //   int8_t error = 0;
  //   Wire.beginTransmission(addr);
  //   error = Wire.endTransmission();
  //   if (error == 0) {
  //     char msg[20];
  //     sprintf(msg, "at %02X", addr);
  //     Serial.println(msg);
  //     devices++;
  //   } else {
  //     char msg[20];
  //     sprintf(msg, "error: %d", error);
  //     Serial.println(msg);
  //   }
  // }
  // char msg[20];
  // sprintf(msg, "%d found", devices);
  // Serial.println(msg);

  tft.init(320, 240);
  tft.fillScreen(BLACK);
  // tft.setTextSize(1);
  // tft.setTextColor(ILI9341_WHITE);

  assert(ts.begin());
  ts.calibrate(cal);

  // draw the crosshairs on screen only once
  for (int i = 0; i < sizeof(_screenPoint) / sizeof(_screenPoint[0]); i++) {
    crosshair(_screenPoint[i]);
  }

  // pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  // pinMode(PRI_BUTTON_PIN, INPUT_PULLDOWN);
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
  alarms.setup();
  motor.setup();
  // assert(preferences.save_into_fs() == 0);

  if (WiFi.status() == WL_CONNECTED) {
    assert(webserver.setup(&alarms, &motor, &tft) == 0);
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

  // static long touch_irq_tk;
  // if (touch.tirqTouched()) {
  //   TS_Point p = touch.getPoint();
  //   ESP_LOGD(TAG, "touch irq pin: %d, %d :: %d", p.x, p.y, p.z);
  //   delay(100);
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
  if (bounce(&touched_tk, 200) && touched()) {

    TS_Point p = ts.getPoint();

    //updateScreenEdges(p);

    // determine which screen point is closest to this touch event
    PointID n = nearestScreenPoint(p);

    // update the corresponding line mapping
    drawMapping(n, p);
  }
}

// -- UTILITY ROUTINES (DEFINITION) --

inline bool touched() {
  return ts.touched();
}

void crosshair(TS_Point p) {
  tft.fillRect(p.x, p.y, 4, 4, WHITE);
  tft.drawFastHLine(p.x - 4, p.y, 9, WHITE);
  tft.drawFastVLine(p.x, p.y - 4, 9, WHITE);
}

uint16_t distance(TS_Point a, TS_Point b) {
  // calculate the distance between points a and b in whatever 2D coordinate
  // system they both exist. returns an integer distance with naive rounding.
  static uint16_t const MAX = ~((uint16_t)0U);
  int32_t dx = b.x - a.x;
  int32_t dy = b.y - a.y;
  uint32_t dsq = (uint32_t)sq(dx) + (uint32_t)sq(dy);
  double d = sqrt(dsq);  // add 1/2 for rounding
  if (d > ((double)MAX - 0.5)) {
    return MAX;
  } else {
    return (uint16_t)(d + 0.5);
  }  // poor-man's rounding
}

void drawMapping(PointID n, TS_Point tp) {

  // static uint8_t const BUF_LEN = 64;
  // static char buf[BUF_LEN] = {'\0'};
  //
  // static uint16_t lineHeight = (uint16_t)(1.5F * 8.0F + 0.5F);
  // static uint16_t lineSpace = 1U;
  //
  // int16_t posX, posY;
  // uint16_t sizeW, sizeH;
  // uint16_t posLeft = 6U;
  // uint16_t posTop =
  //     SCREEN_HEIGHT - (3U - (uint16_t)n) * (lineHeight + lineSpace);
  //
  // TS_Point sp = _screenPoint[(int)n];

  // construct the line buffer
  // snprintf(buf, BUF_LEN, "%c (%u,%u) = (%u,%u)", (uint8_t)n + 'A', sp.x, sp.y,
  // tp.x, tp.y);

  // print the current line to serial port for debugging
  // Serial.printf("%s\n", buf);

  // erase the previous line
  // tft.getTextBounds(buf, posLeft, posTop, &posX, &posY, &sizeW, &sizeH);
  // tft.fillRect(tp.x - 2, tp.y - 2, 5, 5, GREEN);
  //
  // what? TODO FIXME
  // tft.drawFastHLine(0, tp.y, 240, GREEN);
  // tft.drawFastVLine(tp.x, 0, 320, GREEN);
  // tft.drawPixel(tp.x, tp.y, WHITE);
  // tft.fillRect(tp.x, tp.y, 4, 4, WHITE);

  // draw the current line
  // tft.setCursor(posLeft, posTop);
  // tft.printf("%s", buf);
}

// -- NOT FOR GENERAL USAGE -- IGNORE THESE --------------------------- BEGIN --
// approximate calibration only used for determining distance from touch to
// crosshair while calibrating. you can determine these values by using the
// updateScreenEdges() routine below -- just call it with the position of every
// touch event while scrubbing all 4 edges of the screen with a stylus.
#define XPT2046_X_LO 3963
#define XPT2046_X_HI 338
#define XPT2046_Y_LO 4031
#define XPT2046_Y_HI 211
#define MAP_2D_PORTRAIT(x, y)                                              \
  TS_Point((int16_t)map((x), XPT2046_X_LO, XPT2046_X_HI, 0, SCREEN_WIDTH), \
           (int16_t)map((y), XPT2046_Y_LO, XPT2046_Y_HI, 0, SCREEN_HEIGHT))
#define MAP_2D_LANDSCAPE(x, y)                                             \
  TS_Point((int16_t)map((x), XPT2046_Y_LO, XPT2046_Y_HI, 0, SCREEN_WIDTH), \
           (int16_t)map((y), XPT2046_X_LO, XPT2046_X_HI, 0, SCREEN_HEIGHT))

void updateScreenEdges(TS_Point p) {
  static uint16_t xHi = 0xFFFF;
  static uint16_t yHi = 0xFFFF;
  static uint16_t xLo = 0x0;
  static uint16_t yLo = 0x0;
  if (p.x < xHi) {
    xHi = p.x;
  }
  if (p.x > xLo) {
    xLo = p.x;
  }
  if (p.y < yHi) {
    yHi = p.y;
  }
  if (p.y > yLo) {
    yLo = p.y;
  }
  Serial.printf("[X_LO, X_HI] = [%u, %u], [Y_LO, Y_HI] = [%u, %u]\n", xLo, xHi,
                yLo, yHi);
}

// -- NOT FOR GENERAL USAGE -- IGNORE THESE ----------------------------- END --

PointID nearestScreenPoint(TS_Point touch) {

  // #ifdef VERIFY_CALIBRATION
  // the input point is already in screen coordinates because the touchscreen
  // has been calibrated. no need to perform translation.
  TS_Point tp = touch;
  // #else
  // translate the input point (in touch coordinates) to screen coordinates
  // using the hardcoded ranges defined in these macros. not particularly
  // accurate, but it doesn't need to be.
  // TS_Point tp = (SCREEN_ROTATION & 1U) ? MAP_2D_LANDSCAPE(touch.x, touch.y)
  // : MAP_2D_PORTRAIT(touch.x, touch.y);
  // Serial.printf("Touch{%u, %u} => Screen{%u, %u}\n", touch.x, touch.y, tp.x,
  // tp.y);
  // #endif

  PointID near = PointID::NONE;
  uint16_t dist = 0U;

  for (int id = (int)PointID::NONE + 1; id < (int)PointID::COUNT; ++id) {
    // compute the distance from our (translated) input touch point to each
    // screen point to determine minimum distance.
    uint16_t d = distance(tp, _screenPoint[id]);
    if ((near == PointID::NONE) || (d < dist)) {
      // new minimum distance, this is the nearest point to our touch (so far)
      near = (PointID)id;
      dist = d;
    }
  }

  return near;
}
