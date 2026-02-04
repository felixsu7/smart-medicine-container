#ifndef SMC_LUI_H
#define SMC_LUI_H

#include <pins.h>
#include <thirdparty/ST7789V.h>
#include <thirdparty/XPT2046.h>

const int LUI_UI_STACK_SIZE = 8;
const int LUI_COMMAND_STACK_SIZE = 128;
const int LUI_TEXT_STACK_SIZE = 32;
const int LUI_TEXTURE_STACK_SIZE = 32;

// Bits [3:0]- Type of command (SetBrush, DrawRect, DrawCircle, DrawTriangle, DrawTexture, DrawText)
//
// SetBrush sets the "cursor" position of next commands, as well as setting the color. It can also be used to set clipping/limiting of next draw commands, ensuring contents get drawn within boundaries. They are combined because clipping is quite rare operation to do to be honest.
// SetBrush : Flags [7:4] (B0: Clipping command, B1: Change X / unused, B2: Change Y / Y is unused, B3: Change Color / Toggle clipping, LSB First)
// SetBrush : X Position / Clip Width [23:8]
// SetBrush : Y Position / Clip Height [39:24]
// SetBrush : Color / unused [63:40]
//
// DrawRect : Flags [7:4]
// DrawRect : Width [23:8]
// DrawRect : Height [39:24]
// DrawRect : Color [63:40]
//
// DrawCircle : Same as DrawRect [63:4]
//
// DrawTriangle : Orientation [5:4] (0: To left, 1: To right, 2: To top, 3: To bottom)
// DrawTriangle : Unused [7:6]
// DrawTriangle : Same as DrawRect [63:8]
//
// DrawTexture : Unused [7:4]
// DrawTexture : Texture ID [15:8] (See notes on texture storage)
//
// DrawText : Unused [7:4]
// DrawText : Text ID [15:8] (See notes on text storage)

typedef uint64_t LUI_Command;
typedef uint8_t LUI_Sense;

struct LUI_Spatial {
  int16_t x = -1;
  int16_t y = -1;
  int16_t w = -1;
  int16_t h = -1;
};

struct LUI_Padding {
  uint8_t left;
  uint8_t right;
  uint8_t top;
  uint8_t bottom;
};

struct LUI_Theme {
  uint8_t text_size;
  uint8_t text_font;

  uint32_t primary_fg;
  uint32_t secondary_fg;

  uint32_t primary_bg;
  uint32_t secondary_bg;
  uint32_t main_bg;

  LUI_Padding button_padding;
  LUI_Padding label_padding;
};

struct LUI_Texture {
  LUI_Spatial spatial;
  uint8_t* data;
  uint8_t flags;
};

struct LUI_Text {
  LUI_Spatial spatial;
  char* data;
  uint8_t font;
  uint8_t size;
};

struct LUI_Stack {};

class LUI {
  int init(uint8_t tft_dc = TFT_DC, uint8_t tft_cs = TFT_CS,
           uint8_t touch_cs = TOUCH_CS);

  /*!
   * @brief Execute drawing commands to the display
   */
  void present(void);

  uint8_t beginLayout(LUI_Spatial s);
  uint8_t endLayout(void);
  uint8_t setTheme(LUI_Theme theme);

  void setBorder(uint8_t thickness = 0);
  void setTextStyle(uint8_t size, uint32_t fg = COLOR_WHITE,
                    uint32_t bg = COLOR_GRAY);

  LUI_Sense sensing(LUI_Spatial s);
  LUI_Sense rect(LUI_Spatial s);
  LUI_Sense circle(LUI_Spatial s);
  LUI_Sense triangle(LUI_Spatial s, uint8_t orientation);
  LUI_Sense texture(LUI_Texture t);
  LUI_Sense label(LUI_Spatial s, const char* label);
  LUI_Sense button(LUI_Spatial s, const char* label);

 private:
  LUI_Stack ui_stack[LUI_UI_STACK_SIZE];
  int ui_sp;
  LUI_Command command_stack[LUI_COMMAND_STACK_SIZE];
  int command_sp;
  LUI_Text text_stack[LUI_TEXT_STACK_SIZE];
  int text_sp;
  LUI_Texture texture_stack[LUI_TEXTURE_STACK_SIZE];
  int texture_sp;

  ST7789V tft;
  XPT2046 ts;
};

#endif
