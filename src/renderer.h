#ifndef RENDERER_H
#define RENDERER_H

#include "thirdparty/ST7789V.h"
#include "thirdparty/XPT2046.h"
#include "thirdparty/microui.h"

class Renderer {
 public:
  void init(ST7789V* tft, XPT2046* ts);
  void present(bool touched, int touch_x = 0, int touch_y = 0);
  mu_Context muCtx;

 private:
  ST7789V* tft;
  XPT2046* ts;

  void drawRect(mu_Rect rect, mu_Color color);
  void drawText(const char* text, mu_Vec2 pos, mu_Color color);
  void drawIcon(int id, mu_Rect rect, mu_Color color);
  void setClipRect(mu_Rect rect);
  void clear(mu_Color color);

  mu_Rect clipRect;
};

#endif
