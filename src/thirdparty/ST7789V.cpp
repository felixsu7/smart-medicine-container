#include "ST7789V.h"
#include "SPI.h"

static const uint8_t init_commands[] = {
    7,  // 6 commands in list:
        //
    ST7789_SWRESET, ST_CMD_DELAY, 150,

    ST7789_SLPOUT, ST_CMD_DELAY, 255,

    ST7789_COLMOD, 1 + ST_CMD_DELAY, 0b01010101, 10,

    ST7789_MADCTL, 1, 0b10100000,

    // ST7789_CASET, 4, 0x00, 0, (ST7789_TFTWIDTH + 0) >> 8,
    // (ST7789_TFTWIDTH + 0) & 0xFF,

    // ST7789_RASET, 4, 0x00, 0, (ST7789_TFTHEIGHT + 0) >> 8,
    // (ST7789_TFTHEIGHT + 0) & 0xFF,

    ST7789_NORON, ST_CMD_DELAY, 10,

    ST7789_DISPON, ST_CMD_DELAY, 255};

static SPISettings TFT_SPISettings(24000000, MSBFIRST, SPI_MODE0);

ST7789V::ST7789V(int8_t dc, int8_t cs) : Adafruit_GFX(320, 240) {
  _cs = cs;
  _dc = dc;
}

void ST7789V::displayInit(const uint8_t* commands) {
  uint8_t numCommands, numArgs;
  uint16_t ms;
  DC_HIGH();

  int idx = 0;
  numCommands = commands[idx++];    // Number of commands to follow
  while (numCommands--) {           // For each command...
    writecommand(commands[idx++]);  //   Read, issue command
    numArgs = commands[idx++];      //   Number of args to follow
    ms = numArgs & ST_CMD_DELAY;    //   If hibit set, delay follows args
    numArgs &= ~ST_CMD_DELAY;       //   Mask out delay bit
    while (numArgs--) {             //   For each argument...
      writedata(commands[idx++]);   //     Read, issue argument
    }

    if (ms) {
      ms = commands[idx++];  // Read post-command delay time (ms)
      if (ms == 255)
        ms = 500;  // If 255, delay for 500 ms
      delay(ms);
    }
  }
}

void ST7789V::writeAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  uint32_t xa = ((uint32_t)x << 16) | (x + w - 1);
  uint32_t ya = ((uint32_t)y << 16) | (y + h - 1);

  DC_LOW();
  SPI.transfer(ST7789_CASET);  // Column addr set
  DC_HIGH();
  SPI.transfer32(xa);

  DC_LOW();
  SPI.transfer(ST7789_RASET);  // Row addr set
  DC_HIGH();
  SPI.transfer32(ya);

  DC_LOW();
  SPI.transfer(ST7789_RAMWR);  // write to RAM
  DC_HIGH();
}

void ST7789V::setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  startWrite();
  writeAddrWindow(x, y, w, h);
  endWrite();
  // uint16_t x_start = x0, x_end = x1;
  // uint16_t y_start = y0, y_end = y1;
  //
  // writecommand(ST7789_CASET);  // Column addr set
  // writedata(x_start >> 8);
  // writedata(x_start & 0xFF);  // XSTART
  // writedata(x_end >> 8);
  // writedata(x_end & 0xFF);  // XEND
  //
  // writecommand(ST7789_RASET);  // Row addr set
  // writedata(y_start >> 8);
  // writedata(y_start & 0xFF);  // YSTART
  // writedata(y_end >> 8);
  // writedata(y_end & 0xFF);  // YEND
  //
  // writecommand(ST7789_RAMWR);  // write to RAM
}

inline void ST7789V::writeColor(uint16_t color) {
  SPI.transfer16(color);
}

inline void ST7789V::writeColor(uint8_t r5, uint8_t g6, uint8_t b5) {
  SPI.transfer(r5 << 2);
  SPI.transfer(g6 << 2);
  SPI.transfer(b5 << 2);
}

void ST7789V::writePixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height))
    return;

  writeAddrWindow(x, y, 2, 2);

  SPI.transfer16(color);
}

void ST7789V::drawPixel(int16_t x, int16_t y, uint16_t color) {
  startWrite();
  writePixel(x, y, color);
  endWrite();
}

void ST7789V::writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  if ((x >= _width) || (y >= _height))
    return;
  if ((x + w - 1) >= _width)
    w = _width - x;
  writeAddrWindow(x, y, w, 1);

  while (w--) {
    SPI.transfer16(color);
  }
}

void ST7789V::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  startWrite();
  writeFastHLine(x, y, w, color);
  endWrite();
}

void ST7789V::writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  if ((x >= _width) || (y >= _height))
    return;
  if ((y + h - 1) >= _height)
    h = _height - y;
  writeAddrWindow(x, y, 1, h);

  while (h--) {
    SPI.transfer16(color);
  }
}

void ST7789V::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  startWrite();
  writeFastVLine(x, y, h, color);
  endWrite();
}

void ST7789V::drawImage(int16_t x, int16_t y, int16_t w, int16_t h,
                        uint16_t* img) {
  startWrite();
  writeAddrWindow(x, y, w + 1, h + 1);
  // TODO FIXME very inefficent, use non-blocking DMA with callbacks in the future
  for (int i = 0; i < (w + 1) * (h + 1); i++) {
    SPI.transfer16(img[i]);
  }
  endWrite();
}

void ST7789V::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint16_t color) {
  // rudimentary clipping (drawChar w/big text requires this)
  if (x >= _width || y >= _height || w <= 0 || h <= 0) {
    return;
  }

  if (x + w - 1 >= _width)
    w -= (x + w - 1) - _width;
  if (y + h - 1 >= _height)
    h -= (y + h - 1) - _height;

  writeAddrWindow(x, y, w, h);

  for (int i = w * h; i > 0; i--) {
    SPI.transfer16(color);
  }
}

void ST7789V::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                       uint16_t color) {
  startWrite();
  writeFillRect(x, y, w, h, color);
  endWrite();
}

void ST7789V::fillScreen(uint16_t color) {
  fillRect(0, 0, _width, _height, color);
}

// Pass 8-bit (each) R,G,B, get back 18-bit dithered color (TODO)
uint32_t ST7789V::ditherColor(uint8_t r, uint8_t g, uint8_t b) {
  return ((((r) & 0xFC00) << 16) | (((g) & 0xFC00) << 8) | ((b & 0xFC)));
}

void ST7789V::invertDisplay(boolean i) {
  writecommand(i ? ST7789_INVON : ST7789_INVOFF);
}

inline void ST7789V::CS_HIGH(void) {
  digitalWrite(_cs, HIGH);
}

inline void ST7789V::CS_LOW(void) {
  digitalWrite(_cs, LOW);
}

inline void ST7789V::DC_HIGH(void) {
  digitalWrite(_dc, HIGH);
}

inline void ST7789V::DC_LOW(void) {
  digitalWrite(_dc, LOW);
}

void ST7789V::init(uint16_t width, uint16_t height) {
  _width = width;
  _height = height;

  pinMode(_dc, OUTPUT);
  pinMode(_cs, OUTPUT);

  SPI.begin(18, 19, 23, -1);
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  CS_LOW();

  displayInit(init_commands);
}

void ST7789V::partialDisplay(boolean mode) {
  writecommand(mode ? ST7789_PTLON : ST7789_NORON);
}

void ST7789V::sleepDisplay(boolean mode) {
  writecommand(mode ? ST7789_SLPIN : ST7789_SLPOUT);
  delay(5);
}

void ST7789V::enableDisplay(boolean mode) {
  writecommand(mode ? ST7789_DISPON : ST7789_DISPOFF);
}

void ST7789V::idleDisplay(boolean mode) {
  writecommand(mode ? ST7789_IDMON : ST7789_IDMOFF);
}

void ST7789V::resetDisplay() {
  writecommand(ST7789_SWRESET);
  delay(5);
}

void ST7789V::setScrollArea(uint16_t tfa, uint16_t bfa) {
  uint16_t vsa = 240 - tfa - bfa;  // ST7789 320x240 VRAM
  writecommand(ST7789_VSCRDEF);    // SETSCROLLAREA = 0x33
  writedata(tfa >> 8);
  writedata(tfa);
  writedata(vsa >> 8);
  writedata(vsa);
  writedata(bfa >> 8);
  writedata(bfa);
}

void ST7789V::setScroll(uint16_t vsp) {
  writecommand(ST7789_VSCRSADD);  // VSCRSADD = 0x37
  writedata(vsp >> 8);
  writedata(vsp);
}

void ST7789V::setPartArea(uint16_t sr, uint16_t er) {
  writecommand(ST7789_PTLAR);  // SETPARTAREA = 0x30
  writedata(sr >> 8);
  writedata(sr);
  writedata(er >> 8);
  writedata(er);
}

void ST7789V::setBrightness(uint8_t br) {
  //writecommand(ST7789_WRCACE);
  //writedata(0xb1);  // 80,90,b0, or 00,01,02,03
  //writecommand(ST7789_WRCABCMB);
  //writedata(120);

  //BCTRL=0x20, dd=0x08, bl=0x04
  int val = 0x04;
  writecommand(ST7789_WRCTRLD);
  writedata(val);
  writecommand(ST7789_WRDISBV);
  writedata(br);
}

// ----------------------------------------------------------
// 0 - off
// 1 - idle
// 2 - normal
// 4 - display off
void ST7789V::powerSave(uint8_t mode) {
  if (mode == 0) {
    writecommand(ST7789_POWSAVE);
    writedata(0xec | 3);
    writecommand(ST7789_DLPOFFSAVE);
    writedata(0xff);
    return;
  }
  int is = (mode & 1) ? 0 : 1;
  int ns = (mode & 2) ? 0 : 2;
  writecommand(ST7789_POWSAVE);
  writedata(0xec | ns | is);
  if (mode & 4) {
    writecommand(ST7789_DLPOFFSAVE);
    writedata(0xfe);
  }
}

// ------------------------------------------------
// Input a value 0 to 511 (85*6) to get a color value.
// The colours are a transition R - Y - G - C - B - M - R.
void ST7789V::rgbWheel(int idx, uint8_t* _r, uint8_t* _g, uint8_t* _b) {
  idx &= 0x1ff;
  if (idx < 85) {  // R->Y
    *_r = 255;
    *_g = idx * 3;
    *_b = 0;
    return;
  } else if (idx < 85 * 2) {  // Y->G
    idx -= 85 * 1;
    *_r = 255 - idx * 3;
    *_g = 255;
    *_b = 0;
    return;
  } else if (idx < 85 * 3) {  // G->C
    idx -= 85 * 2;
    *_r = 0;
    *_g = 255;
    *_b = idx * 3;
    return;
  } else if (idx < 85 * 4) {  // C->B
    idx -= 85 * 3;
    *_r = 0;
    *_g = 255 - idx * 3;
    *_b = 255;
    return;
  } else if (idx < 85 * 5) {  // B->M
    idx -= 85 * 4;
    *_r = idx * 3;
    *_g = 0;
    *_b = 255;
    return;
  } else {  // M->R
    idx -= 85 * 5;
    *_r = 255;
    *_g = 0;
    *_b = 255 - idx * 3;
    return;
  }
}

uint32_t ST7789V::rgbWheel(int idx) {
  uint8_t r, g, b;
  rgbWheel(idx, &r, &g, &b);
  return ditherColor(r, g, b);
}

void ST7789V::startWrite(void) {
  SPI.beginTransaction(TFT_SPISettings);
  DC_HIGH();
  CS_LOW();
}

void ST7789V::endWrite(void) {
  CS_HIGH();
  SPI.endTransaction();
}

/*
inline void ST7789V::transfer24(uint32_t c) {
  SPI.transfer(c >> 16);
  SPI.transfer(c >> 8);
  SPI.transfer(c);
}
*/

void ST7789V::writecommand(uint8_t c) {
  DC_LOW();
  CS_LOW();
  SPI.beginTransaction(TFT_SPISettings);

  SPI.transfer(c);

  CS_HIGH();
  SPI.endTransaction();
}

void ST7789V::writedata(uint8_t c) {
  SPI.beginTransaction(TFT_SPISettings);
  DC_HIGH();
  CS_LOW();

  SPI.transfer(c);

  CS_HIGH();
  SPI.endTransaction();
}
