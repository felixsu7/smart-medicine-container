#include "renderer.h"
#include "thirdparty/ST7789V.h"
#include "thirdparty/XPT2046.h"
#include "thirdparty/microui.h"

static const char* TAG = "renderer";

static uint16_t const SCREEN_WIDTH = 320;
static uint16_t const SCREEN_HEIGHT = 240;

static TS_Calibration cal(TS_Point(13, 11), TS_Point(3530, 3465),
                          TS_Point(312, 113), TS_Point(381, 2275),
                          TS_Point(167, 214), TS_Point(2015, 710), SCREEN_WIDTH,
                          SCREEN_HEIGHT);

void Renderer::drawRect(mu_Rect rect, mu_Color color) {
  tft->fillRect(rect.x, rect.y, rect.w, rect.h,
                tft->ditherColor(color.r, color.g, color.b));
};

void Renderer::drawText(const char* text, mu_Vec2 pos, mu_Color color) {
  tft->setCursor(pos.x, pos.y);
  tft->setTextColor(tft->ditherColor(color.r, color.g, color.b));
  tft->print(text);
};

void Renderer::drawIcon(int id, mu_Rect rect, mu_Color color) {
  ESP_LOGD(TAG, "drawIcon %d, [x%d, y%d, w%d, h%d], c%ld", id, rect.x, rect.y,
           rect.w, rect.y, tft->ditherColor(color.r, color.g, color.b));
};

// TODO
void Renderer::setClipRect(mu_Rect rect) {
  ESP_LOGD(TAG, "setClipRect [x%d, y%d, w%d, h%d]", rect.x, rect.y, rect.w,
           rect.h);
  clipRect = rect;
};

void Renderer::clear(mu_Color color) {
  drawRect(mu_rect(0, 0, 320, 240), color);
};

void charBounds(unsigned char c, int16_t* x, int16_t* y, int16_t* minx,
                int16_t* miny, int16_t* maxx, int16_t* maxy, int16_t textsize_x,
                int16_t textsize_y, bool wrap, int16_t screen_width) {
  if (c == '\n') {         // Newline?
    *x = 0;                // Reset x to zero,
    *y += textsize_y * 8;  // advance y one line
    // min/max x/y unchaged -- that waits for next 'normal' character
  } else if (c != '\r') {  // Normal char; ignore carriage returns
    if (wrap && ((*x + textsize_x * 6) > screen_width)) {  // Off right?
      *x = 0;                                              // Reset x to zero,
      *y += textsize_y * 8;                                // advance y one line
    }
    int x2 = *x + textsize_x * 6 - 1,  // Lower-right pixel of char
        y2 = *y + textsize_y * 8 - 1;
    if (x2 > *maxx)
      *maxx = x2;  // Track max x, y
    if (y2 > *maxy)
      *maxy = y2;
    if (*x < *minx)
      *minx = *x;  // Track min x, y
    if (*y < *miny)
      *miny = *y;
    *x += textsize_x * 6;  // Advance x one char
  }
}

int text_width(mu_Font font, const char* text, int len) {
  uint8_t c;  // Current character
  int x, w = 0;
  int16_t minx = 0x7FFF, miny = 0x7FFF, maxx = -1, maxy = -1;  // Bound rect
  // Bound rect is intentionally initialized inverted, so 1st char sets it
  //
  while ((c = *text++) && len--) {
    // charBounds() modifies x/y to advance for each character,
    // and min/max x/y are updated to incrementally build bounding rect.
    charBounds(c, 0, 0, &minx, &miny, &maxx, &maxy, 2, 2, false, 320);
  }

  if (maxx >= minx) {     // If legit string bounds were found...
    x = minx;             // Update x1 to least X coord,
    w = maxx - minx + 1;  // And w to bound rect width
  }

  return w;
};

int text_height(mu_Font font) {
  return 7;
};

void Renderer::init(ST7789V* tft, XPT2046* ts) {
  this->tft = tft;
  tft->init(320, 240);

  this->ts = ts;
  assert(ts->begin());
  ts->calibrate(cal);

  mu_init(&muCtx);
  muCtx.text_width = text_width;
  muCtx.text_height = text_height;
}

void Renderer::present(bool touched, int touch_x, int touch_y) {
  if (touched) {
    mu_input_mousedown(&muCtx, touch_x, touch_y, MU_MOUSE_LEFT);
  }

  clear(mu_color(0, 0, 0, 255));
  mu_Command* cmd = NULL;
  while (mu_next_command(&muCtx, &cmd)) {
    switch (cmd->type) {
      case MU_COMMAND_TEXT:
        drawText(cmd->text.str, cmd->text.pos, cmd->text.color);
        break;
      case MU_COMMAND_RECT:
        drawRect(cmd->rect.rect, cmd->rect.color);
        break;
      case MU_COMMAND_ICON:
        drawIcon(cmd->icon.id, cmd->icon.rect, cmd->icon.color);
        break;
      case MU_COMMAND_CLIP:
        setClipRect(cmd->clip.rect);
        break;
    }
  }
}
