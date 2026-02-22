/***************************************************
  This is a library for the ST7789 IPS SPI display.

  Originally written by Limor Fried/Ladyada for 
  Adafruit Industries.

  Modified by Ananev Ilia
  Modified by Kamran Gasimov
 ****************************************************/

#ifndef _ADAFRUIT_ST7789H_
#define _ADAFRUIT_ST7789H_

#include "Adafruit_GFX.h"
#include "Arduino.h"

#define ST_CMD_DELAY 0x80

#define ST7789_NOP 0x00
#define ST7789_SWRESET 0x01

#define ST7789_SLPIN 0x10    // sleep on
#define ST7789_SLPOUT 0x11   // sleep off
#define ST7789_PTLON 0x12    // partial on
#define ST7789_NORON 0x13    // partial off
#define ST7789_INVOFF 0x20   // invert off
#define ST7789_INVON 0x21    // invert on
#define ST7789_DISPOFF 0x28  // display off
#define ST7789_DISPON 0x29   // display on
#define ST7789_IDMOFF 0x38   // idle off
#define ST7789_IDMON 0x39    // idle on

#define ST7789_CASET 0x2A
#define ST7789_RASET 0x2B
#define ST7789_RAMWR 0x2C
#define ST7789_RAMRD 0x2E

#define ST7789_COLMOD 0x3A
#define ST7789_MADCTL 0x36

#define ST7789_PTLAR 0x30    // partial start/end
#define ST7789_VSCRDEF 0x33  // SETSCROLLAREA
#define ST7789_VSCRSADD 0x37

#define ST7789_WRDISBV 0x51
#define ST7789_WRCTRLD 0x53
#define ST7789_WRCACE 0x55
#define ST7789_WRCABCMB 0x5e

#define ST7789_POWSAVE 0xbc
#define ST7789_DLPOFFSAVE 0xbd

// bits in MADCTL
#define ST7789_MADCTL_MY 0x80
#define ST7789_MADCTL_MX 0x40
#define ST7789_MADCTL_MV 0x20
#define ST7789_MADCTL_ML 0x10
#define ST7789_MADCTL_RGB 0x00

// Color definitions
// A color is 18bit but ST7789V accepts these as 24bit values, ignoring the 2 least significant bits per byte.
// FC is 0b1111110, 7C is 01111100 (half of FC)
#define COLOR_BLACK 0x000000
#define COLOR_RED 0xFC0000
#define COLOR_ROSE 0xFC007C
#define COLOR_MAGENTA 0xFC00FC
#define COLOR_VIOLET 0x7C00FC
#define COLOR_BLUE 0x0000FC
#define COLOR_AZURE 0x007CFC
#define COLOR_CYAN 0x00FCFC
#define COLOR_SPRING_GREEN 0x00FC7C
#define COLOR_GREEN 0x00FC00
#define COLOR_CHARTREUSE 0x7CFC00
#define COLOR_YELLOW 0xFCFC00
#define COLOR_ORANGE 0xFC7C00
#define COLOR_WHITE 0xFCFCFC
#define COLOR_GRAY 0x7C7C7C
#define COLOR_LIGHTGRAY 0x2F2F2F
#define COLOR_DARKGRAY 0x101010

class ST7789V : public Adafruit_GFX {
 public:
  ST7789V(int8_t DC, int8_t CS);

  void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  void writeAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h),
      writeColor(uint16_t color),
      writeColor(uint8_t r5, uint8_t g6, uint8_t b5),
      fillScreen(uint16_t color),
      drawPixel(int16_t x, int16_t y, uint16_t color),
      drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color),
      drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color),
      fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color),
      invertDisplay(boolean i), init(uint16_t width, uint16_t height);
  uint32_t ditherColor(uint8_t r, uint8_t g, uint8_t b);

  void drawImage(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t* img);

  void startWrite(void);
  void writePixel(int16_t x, int16_t y, uint16_t color);
  void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                     uint16_t color);
  void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  // void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
  // uint16_t color);
  void endWrite(void);

  void partialDisplay(boolean mode);
  void sleepDisplay(boolean mode);
  void enableDisplay(boolean mode);
  void idleDisplay(boolean mode);
  void resetDisplay();
  void setScrollArea(uint16_t tfa, uint16_t bfa);
  void setScroll(uint16_t vsp);
  void setPartArea(uint16_t sr, uint16_t er);
  void setBrightness(uint8_t br);
  void powerSave(uint8_t mode);

  void rgbWheel(int idx, uint8_t* _r, uint8_t* _g, uint8_t* _b);
  uint32_t rgbWheel(int idx);

  void displayInit(const uint8_t* addr), spiwrite(uint8_t),
      writecommand(uint8_t c), writedata(uint8_t d);

 private:
  inline void CS_HIGH(void);
  inline void CS_LOW(void);
  inline void DC_HIGH(void);
  inline void DC_LOW(void);

  uint16_t _width, _height;

  int8_t _cs, _dc;
};

#endif
