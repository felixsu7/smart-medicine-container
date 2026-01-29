#ifndef UI_H
#define UI_H

#include <thirdparty/ST7789v_arduino.h>
#include <thirdparty/XPT2046_Calibrated.h>

class UIContext {
 public:
  int begin(ST7789v_arduino* display, XPT2046_Calibrated* touch);
  void drawPixel(int x, int y, uint16_t color);
  void drawHLine(int x, int y, int w, uint16_t color);
  void drawVLine(int x, int y, int h, uint16_t color);
  void drawRect(int x, int y, int w, int h, uint16_t color);
  inline uint16_t ditherRGB(uint8_t r, uint8_t g, uint8_t b);
  void draw1BitImage(int x, int y, int w, int h, const uint8_t* image);

  void draw2BitImage(int x, int y, int w, int h, const uint8_t* image);

  void draw4BitImage(int x, int y, int w, int h, const uint8_t* image);

  void draw8BitImage(int x, int y, int w, int h, const uint8_t* image);

  void draw12BitImage(int x, int y, int w, int h, const uint8_t* image);

 private:
  ST7789v_arduino* display;
  XPT2046_Calibrated* touch;
};

#endif
