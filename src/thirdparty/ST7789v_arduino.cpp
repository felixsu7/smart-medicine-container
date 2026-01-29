/***************************************************
  This is a library for the ST7789 IPS SPI display.

  Originally written by Limor Fried/Ladyada for 
  Adafruit Industries.

  Modified by Ananev Ilia
  Modified by Kamran Gasimov
 ****************************************************/

// TODO FIXME heavy refactoring
#include "ST7789v_arduino.h"
#include <limits.h>
#include "SPI.h"
// #include <PI.h >

static const uint8_t init_commands[] =
    {        // Initialization commands for 7789 screens
        10,  // 9 commands in list:
        ST7789_SWRESET,
        ST_CMD_DELAY,  // 1: Software reset, no args, w/delay
        150,           // 150 ms delay
        ST7789_SLPOUT,
        ST_CMD_DELAY,  // 2: Out of sleep mode, no args, w/delay
        255,           // 255 = 500 ms delay
        ST7789_COLMOD,
        1 + ST_CMD_DELAY,  // 3: Set color mode, 1 arg + delay:
        0x53,              // 12-bit color
        10,                // 10 ms delay
        ST7789_MADCTL,
        1,     // 4: Memory access ctrl (directions), 1 arg:
        0x00,  // Row addr/col addr, bottom to top refresh
        ST7789_CASET,
        4,  // 5: Column addr set, 4 args, no delay:
        0x00,
        ST7789_240x240_XSTART,  // XSTART = 0
        (ST7789_TFTWIDTH + ST7789_240x240_XSTART) >> 8,
        (ST7789_TFTWIDTH + ST7789_240x240_XSTART) & 0xFF,  // XEND = 240
        ST7789_RASET,
        4,  // 6: Row addr set, 4 args, no delay:
        0x00,
        ST7789_240x240_YSTART,  // YSTART = 0
        (ST7789_TFTHEIGHT + ST7789_240x240_YSTART) >> 8,
        (ST7789_TFTHEIGHT + ST7789_240x240_YSTART) & 0xFF,  // YEND = 240
        // ST7789_INVON,
        // ST_CMD_DELAY,  // 7: Inversion ON
        // 10,
        ST7789_NORON,
        ST_CMD_DELAY,  // 8: Normal display on, no args, w/delay
        10,            // 10 ms delay
        ST7789_DISPON,
        ST_CMD_DELAY,  // 9: Main screen turn on, no args, w/delay
        255};          // 255 = 500 ms delay

// inline uint16_t swapcolor(uint16_t x) {
//   return (x << 11) | (x & 0x07E0) | (x >> 11);
// }

static SPISettings TFT_SPISettings(16000000, MSBFIRST, SPI_MODE0);

// Constructor when using hardware SPI.  Faster, but must use SPI pins
// specific to each board type (e.g. 11,13 for Uno, 51,52 for Mega, etc.)
ST7789v_arduino::ST7789v_arduino(int8_t dc, int8_t rst, int8_t cs) {
  _cs = cs;
  _dc = dc;
}

inline void ST7789v_arduino::spiwrite(uint8_t c) {
  SPI.transfer(c);
}

inline void ST7789v_arduino::spiwrite12(uint16_t c) {
  SPI.transferBits(c, NULL, 12);
}

void ST7789v_arduino::writecommand(uint8_t c) {

  DC_LOW();
  CS_LOW();
  SPI.beginTransaction(TFT_SPISettings);

  spiwrite(c);

  CS_HIGH();
  SPI.endTransaction();
}

void ST7789v_arduino::writedata(uint8_t c) {
  SPI.beginTransaction(TFT_SPISettings);
  DC_HIGH();
  CS_LOW();

  spiwrite(c);

  CS_HIGH();
  SPI.endTransaction();
}

// Companion code to the above tables.  Reads and issues
// a series of LCD commands stored in PROGMEM byte array.
void ST7789v_arduino::displayInit(const uint8_t* addr) {

  uint8_t numCommands, numArgs;
  uint16_t ms;
  //<-----------------------------------------------------------------------------------------
  DC_HIGH();
  //<-----------------------------------------------------------------------------------------

  numCommands = pgm_read_byte(addr++);    // Number of commands to follow
  while (numCommands--) {                 // For each command...
    writecommand(pgm_read_byte(addr++));  //   Read, issue command
    numArgs = pgm_read_byte(addr++);      //   Number of args to follow
    ms = numArgs & ST_CMD_DELAY;          //   If hibit set, delay follows args
    numArgs &= ~ST_CMD_DELAY;             //   Mask out delay bit
    while (numArgs--) {                   //   For each argument...
      writedata(pgm_read_byte(addr++));   //     Read, issue argument
    }

    if (ms) {
      ms = pgm_read_byte(addr++);  // Read post-command delay time (ms)
      if (ms == 255)
        ms = 500;  // If 255, delay for 500 ms
      delay(ms);
    }
  }
}

// Initialization code common to all ST7789 displays
void ST7789v_arduino::commonInit(const uint8_t* cmdList) {
  _ystart = _xstart = 0;
  _colstart = _rowstart = 0;  // May be overridden in init func

  pinMode(_dc, OUTPUT);
  if (_cs) {
    pinMode(_cs, OUTPUT);
  }

  SPI.begin(18, 19, 23, -1);
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  // toggle RST low to reset; CS low so it'll listen to us
  CS_LOW();

  if (cmdList)
    displayInit(cmdList);
}

void ST7789v_arduino::setRotation(uint8_t m) {

  writecommand(ST7789_MADCTL);
  rotation = m % 4;  // can't be higher than 3
  switch (rotation) {
    case 0:
      writedata(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);

      _xstart = _colstart;
      _ystart = _rowstart;
      break;
    case 1:
      writedata(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);

      _ystart = _colstart;
      _xstart = _rowstart;
      break;
    case 2:
      writedata(ST7789_MADCTL_RGB);

      _xstart = _colstart;
      _ystart = _rowstart;
      break;

    case 3:
      writedata(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);

      _ystart = _colstart;
      _xstart = _rowstart;
      break;
  }
}

void ST7789v_arduino::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1,
                                    uint16_t y1) {

  uint16_t x_start = x0 + _xstart, x_end = x1 + _xstart;
  uint16_t y_start = y0 + _ystart, y_end = y1 + _ystart;

  writecommand(ST7789_CASET);  // Column addr set
  writedata(x_start >> 8);
  writedata(x_start & 0xFF);  // XSTART
  writedata(x_end >> 8);
  writedata(x_end & 0xFF);  // XEND

  writecommand(ST7789_RASET);  // Row addr set
  writedata(y_start >> 8);
  writedata(y_start & 0xFF);  // YSTART
  writedata(y_end >> 8);
  writedata(y_end & 0xFF);  // YEND

  writecommand(ST7789_RAMWR);  // write to RAM
}

// void ST7789v_arduino::pushColor(uint16_t color) {
//   SPI_BEGIN_TRANSACTION();
//   DC_HIGH();
//   CS_LOW();
//
//   spiwrite(color >> 8);
//   spiwrite(color);
//
//   CS_HIGH();
//   SPI_END_TRANSACTION();
// }

void ST7789v_arduino::drawPixel(int16_t x, int16_t y, uint16_t color) {

  if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height))
    return;

  setAddrWindow(x, y, x + 1, y + 1);

  SPI.beginTransaction(TFT_SPISettings);
  DC_HIGH();
  CS_LOW();

  spiwrite12(color);

  CS_HIGH();
  SPI.endTransaction();
}

void ST7789v_arduino::drawFastVLine(int16_t x, int16_t y, int16_t h,
                                    uint16_t color) {

  // Rudimentary clipping
  if ((x >= _width) || (y >= _height))
    return;
  if ((y + h - 1) >= _height)
    h = _height - y;
  setAddrWindow(x, y, x, y + h - 1);

  SPI.beginTransaction(TFT_SPISettings);
  DC_HIGH();
  CS_LOW();

  while (h--) {
    spiwrite12(color);
  }

  CS_HIGH();
  SPI.endTransaction();
}

void ST7789v_arduino::drawFastHLine(int16_t x, int16_t y, int16_t w,
                                    uint16_t color) {

  // Rudimentary clipping
  if ((x >= _width) || (y >= _height))
    return;
  if ((x + w - 1) >= _width)
    w = _width - x;
  setAddrWindow(x, y, x + w - 1, y);

  SPI.beginTransaction(TFT_SPISettings);
  DC_HIGH();
  CS_LOW();

  while (w--) {
    spiwrite12(color);
  }

  CS_HIGH();
  SPI.endTransaction();
}

void ST7789v_arduino::fillScreen(uint16_t color) {
  fillRect(0, 0, _width, _height, color);
}

// // fill a rectangle
// void ST7789v_arduino::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
//   uint16_t color) {

//   // rudimentary clipping (drawChar w/big text requires this)
//   if((x >= _width) || (y >= _height)) return;
//   if((x + w - 1) >= _width)  w = _width  - x;
//   if((y + h - 1) >= _height) h = _height - y;

//   setAddrWindow(x, y, x+w-1, y+h-1);

//   uint8_t hi = color >> 8, lo = color;

//   SPI_BEGIN_TRANSACTION();

//   DC_HIGH();
//   CS_LOW();
//   for(y=h; y>0; y--) {
//     for(x=w; x>0; x--) {
//       spiwrite(hi);
//       spiwrite(lo);
//     }
//   }
//   CS_HIGH();
//   SPI_END_TRANSACTION();
// }

// fill a rectangle
void ST7789v_arduino::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                               uint16_t color) {

  // rudimentary clipping (drawChar w/big text requires this)
  if (x >= _width || y >= _height || w <= 0 || h <= 0)
    return;
  if (x + w - 1 >= _width)
    w = _width - x;
  if (y + h - 1 >= _height)
    h = _height - y;
  setAddrWindow(x, y, x + w - 1, y + h - 1);

  SPI.beginTransaction(TFT_SPISettings);

  DC_HIGH();
  CS_LOW();

  uint32_t num = (uint32_t)w * h;
  uint16_t num16 = num >> 4;
  while (num16--) {
    for (int i = 0; i < 32; i++) {
      spiwrite12(color);
    }
  }
  uint8_t num8 = num & 0xf;
  while (num8--) {
    spiwrite12(color);
  }

  CS_HIGH();
  SPI.endTransaction();
}

// Pass 8-bit (each) R,G,B, get back 16-bit packed color
uint16_t ST7789v_arduino::Color444(uint8_t r, uint8_t g, uint8_t b) {
  return ((((r) & 0xF) << 8) | (((g) & 0xF) << 4) | ((b & 0xF)));
}

void ST7789v_arduino::invertDisplay(boolean i) {
  writecommand(i ? ST7789_INVON : ST7789_INVOFF);
}

/******** low level bit twiddling **********/

inline void ST7789v_arduino::CS_HIGH(void) {
  if (_cs) {
    digitalWrite(_cs, HIGH);
  }
}

inline void ST7789v_arduino::CS_LOW(void) {
  if (_cs) {
    digitalWrite(_cs, LOW);
  }
}

inline void ST7789v_arduino::DC_HIGH(void) {
  _DCbit = true;
  digitalWrite(_dc, HIGH);
}

inline void ST7789v_arduino::DC_LOW(void) {
  _DCbit = false;
  digitalWrite(_dc, LOW);
}

void ST7789v_arduino::init(uint16_t width, uint16_t height) {
  commonInit(NULL);

  _colstart = ST7789_240x240_XSTART;
  _rowstart = ST7789_240x240_YSTART;
  _height = 320;
  _width = 240;

  displayInit(init_commands);

  setRotation(2);
}

// ----------------------------------------------------------
// draws image from RAM
void ST7789v_arduino::drawImage(int16_t x, int16_t y, int16_t w, int16_t h,
                                uint16_t* img16) {
  if (x >= _width || y >= _height || w <= 0 || h <= 0)
    return;
  setAddrWindow(x, y, x + w - 1, y + h - 1);

  SPI.beginTransaction(TFT_SPISettings);

  DC_HIGH();
  CS_LOW();

  uint32_t num = (uint32_t)w * h;
  uint16_t num16 = num >> 3;
  uint8_t* img = (uint8_t*)img16;
  while (num16--) {
    spiwrite(*(img + 1));
    spiwrite(*(img + 0));
    img += 2;
    spiwrite(*(img + 1));
    spiwrite(*(img + 0));
    img += 2;
    spiwrite(*(img + 1));
    spiwrite(*(img + 0));
    img += 2;
    spiwrite(*(img + 1));
    spiwrite(*(img + 0));
    img += 2;
    spiwrite(*(img + 1));
    spiwrite(*(img + 0));
    img += 2;
    spiwrite(*(img + 1));
    spiwrite(*(img + 0));
    img += 2;
    spiwrite(*(img + 1));
    spiwrite(*(img + 0));
    img += 2;
    spiwrite(*(img + 1));
    spiwrite(*(img + 0));
    img += 2;
  }
  uint8_t num8 = num & 0x7;
  while (num8--) {
    spiwrite(*(img + 1));
    spiwrite(*(img + 0));
    img += 2;
  }

  CS_HIGH();
  SPI.endTransaction();
}

// ----------------------------------------------------------
// draws image from flash (PROGMEM)
void ST7789v_arduino::drawImageF(int16_t x, int16_t y, int16_t w, int16_t h,
                                 const uint16_t* img16) {
  if (x >= _width || y >= _height || w <= 0 || h <= 0)
    return;
  setAddrWindow(x, y, x + w - 1, y + h - 1);

  SPI.beginTransaction(TFT_SPISettings);

  DC_HIGH();
  CS_LOW();

  uint32_t num = (uint32_t)w * h;
  uint16_t num16 = num >> 3;
  uint8_t* img = (uint8_t*)img16;
  while (num16--) {
    spiwrite(pgm_read_byte(img + 1));
    spiwrite(pgm_read_byte(img + 0));
    img += 2;
    spiwrite(pgm_read_byte(img + 1));
    spiwrite(pgm_read_byte(img + 0));
    img += 2;
    spiwrite(pgm_read_byte(img + 1));
    spiwrite(pgm_read_byte(img + 0));
    img += 2;
    spiwrite(pgm_read_byte(img + 1));
    spiwrite(pgm_read_byte(img + 0));
    img += 2;
    spiwrite(pgm_read_byte(img + 1));
    spiwrite(pgm_read_byte(img + 0));
    img += 2;
    spiwrite(pgm_read_byte(img + 1));
    spiwrite(pgm_read_byte(img + 0));
    img += 2;
    spiwrite(pgm_read_byte(img + 1));
    spiwrite(pgm_read_byte(img + 0));
    img += 2;
    spiwrite(pgm_read_byte(img + 1));
    spiwrite(pgm_read_byte(img + 0));
    img += 2;
  }
  uint8_t num8 = num & 0x7;
  while (num8--) {
    spiwrite(pgm_read_byte(img + 1));
    spiwrite(pgm_read_byte(img + 0));
    img += 2;
  }

  CS_HIGH();
  SPI.endTransaction();
}

// ----------------------------------------------------------
void ST7789v_arduino::partialDisplay(boolean mode) {
  writecommand(mode ? ST7789_PTLON : ST7789_NORON);
}

// ----------------------------------------------------------
void ST7789v_arduino::sleepDisplay(boolean mode) {
  writecommand(mode ? ST7789_SLPIN : ST7789_SLPOUT);
  delay(5);
}

// ----------------------------------------------------------
void ST7789v_arduino::enableDisplay(boolean mode) {
  writecommand(mode ? ST7789_DISPON : ST7789_DISPOFF);
}

// ----------------------------------------------------------
void ST7789v_arduino::idleDisplay(boolean mode) {
  writecommand(mode ? ST7789_IDMON : ST7789_IDMOFF);
}

// ----------------------------------------------------------
void ST7789v_arduino::resetDisplay() {
  writecommand(ST7789_SWRESET);
  delay(5);
}

// ----------------------------------------------------------
void ST7789v_arduino::setScrollArea(uint16_t tfa, uint16_t bfa) {
  uint16_t vsa = 240 - tfa - bfa;  // ST7789 320x240 VRAM
  writecommand(ST7789_VSCRDEF);    // SETSCROLLAREA = 0x33
  writedata(tfa >> 8);
  writedata(tfa);
  writedata(vsa >> 8);
  writedata(vsa);
  writedata(bfa >> 8);
  writedata(bfa);
}

// ----------------------------------------------------------
void ST7789v_arduino::setScroll(uint16_t vsp) {
  writecommand(ST7789_VSCRSADD);  // VSCRSADD = 0x37
  writedata(vsp >> 8);
  writedata(vsp);
}

// ----------------------------------------------------------
void ST7789v_arduino::setPartArea(uint16_t sr, uint16_t er) {
  writecommand(ST7789_PTLAR);  // SETPARTAREA = 0x30
  writedata(sr >> 8);
  writedata(sr);
  writedata(er >> 8);
  writedata(er);
}

// ----------------------------------------------------------
// doesn't work
void ST7789v_arduino::setBrightness(uint8_t br) {
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
void ST7789v_arduino::powerSave(uint8_t mode) {
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
void ST7789v_arduino::rgbWheel(int idx, uint8_t* _r, uint8_t* _g, uint8_t* _b) {
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

uint16_t ST7789v_arduino::rgbWheel(int idx) {
  uint8_t r, g, b;
  rgbWheel(idx, &r, &g, &b);
  return RGBto444(r, g, b);
}

void ST7789v_arduino::startWrite(void) {

  SPI.beginTransaction(TFT_SPISettings);
  CS_LOW();
}

void ST7789v_arduino::endWrite(void) {
  CS_HIGH();
  SPI.endTransaction();
}
