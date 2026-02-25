#ifndef SMC_MENU_H
#define SMC_MENU_H

#ifdef SMC_DESKTOP
#define LVGL_INCLUDE "lvgl/lvgl.h"
#else
#define LVGL_INCLUDE "thirdparty/lvgl/lvgl.h"
#endif

void test_menu(void);

#endif
