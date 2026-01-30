/***************************************************
  This is a library for the ST7789 IPS SPI display.

  Originally written by Limor Fried/Ladyada for 
  Adafruit Industries.

  Modified by Ananev Ilia
  Modified by Kamran Gasimov
 ****************************************************/

#ifndef _ADAFRUIT_ST7789H_
#define _ADAFRUIT_ST7789H_

#include "Arduino.h"

#define ST7789_TFTWIDTH 320
#define ST7789_TFTHEIGHT 240

#define ST7789_240x240_XSTART 0
#define ST7789_240x240_YSTART 0

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
#define BLACK 0x000
#define RED 0xF00
#define ROSE 0xF08
#define MAGENTA 0xF0F
#define VIOLET 0x80F
#define BLUE 0x000F
#define AZURE 0x08F
#define CYAN 0x0FF
#define SPRING_GREEN 0x0F8
#define GREEN 0x0F0
#define CHARTREUSE 0x8F0
#define YELLOW 0xFF0
#define ORANGE 0xF80
#define WHITE 0xFFF

#define RGBto444(r, g, b) \
  ((((r) & 0xF) << 8) | (((g) & 0xF) << 4) | ((b & 0xF)))

class ST7789v_arduino {

 public:
  ST7789v_arduino(int8_t DC, int8_t RST, int8_t CS = -1);

  void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1),
      pushColor(uint16_t color), fillScreen(uint16_t color),
      drawPixel(int16_t x, int16_t y, uint16_t color),
      drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color),
      drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color),
      fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color),
      invertDisplay(boolean i), init(uint16_t width, uint16_t height);
  uint16_t Color444(uint8_t r, uint8_t g, uint8_t b);

  uint16_t color444(uint8_t r, uint8_t g, uint8_t b) {
    return Color444(r, g, b);
  }

  void drawImage(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t* img);
  void drawImageF(int16_t x, int16_t y, int16_t w, int16_t h,
                  const uint16_t* img16);

  void drawImageF(int16_t x, int16_t y, const uint16_t* img16) {
    drawImageF(x, y, pgm_read_word(img16), pgm_read_word(img16 + 1), img16 + 3);
  }

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
  void startWrite();
  void endWrite();

  void rgbWheel(int idx, uint8_t* _r, uint8_t* _g, uint8_t* _b);
  uint16_t rgbWheel(int idx);

 protected:
  uint8_t _colstart, _rowstart, _xstart,
      _ystart;  // some displays need this changed

  void displayInit(const uint8_t* addr);
  void spiwrite(uint8_t), spiwrite12(uint16_t), writecommand(uint8_t c),
      writedata(uint8_t d), commonInit(const uint8_t* cmdList);

 private:
  inline void CS_HIGH(void);
  inline void CS_LOW(void);
  inline void DC_HIGH(void);
  inline void DC_LOW(void);

  boolean _DCbit;

  uint16_t _width, _height;

  int8_t _cs, _dc;
};

#endif
