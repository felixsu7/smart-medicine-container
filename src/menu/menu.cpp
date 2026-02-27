#include "menu.h"
#include <cstdio>
#include "../ui.h"
#include "./alarm.h"
#include "./boot_logo.h"
#include "lvgl_homescreen.h"
#include "stdlib.h"
#include "widgets.h"
#include LVGL_INCLUDE

// TODO
lv_subject_t motor_subject;
lv_subject_t steps_subject;

static void event_handler(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* obj = lv_event_get_target_obj(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    uint32_t id = lv_buttonmatrix_get_selected_button(obj);
    const char* txt = lv_buttonmatrix_get_button_text(obj, id);
    LV_UNUSED(txt);
    LV_LOG_USER("%s was pressed\n", txt);
  }
}

static const char* btnm_map[] = {"1",  "2",  "3",       "4",       "67",
                                 "\n", "6",  "7",       "8",       "9",
                                 "0",  "\n", "Action1", "Action2", ""};

static lv_style_t style;

void smc_style_init(void) {
  lv_style_init(&style);

  lv_style_set_drop_shadow_color(&style, lv_palette_main(LV_PALETTE_RED));
  lv_style_set_drop_shadow_radius(&style, 16);
  lv_style_set_drop_shadow_opa(&style, 255);
  lv_style_set_drop_shadow_offset_x(&style, 5);
  lv_style_set_drop_shadow_offset_y(&style, 10);
}

void test_menu(void) {
  homescreen_create();
  // smc_style_init();
  // lv_obj_t* btnm = lv_buttonmatrix_create(lv_screen_active());
  // lv_buttonmatrix_set_map(btnm, btnm_map);
  // lv_obj_set_size(btnm, lv_pct(100), lv_pct(100));
}

void smc_internal_loop(void) {
  lv_subject_set_int(&steps_subject, smc_motor_steps() * 100 / 4096);
  static long alarm_ptr_tk;
  // if (bounce_alt(&alarm_ptr_tk)) {
  //   alarms_page_refresh(alarms_page);
  // }

  Alarms* alarms = smc_system_alarms();

  if (alarms->is_ringing() > -1) {
    smc_alarm_buzzer_play(SMC_Melody{});
  } else {
    smc_alarm_buzzer_off();
  }
};
