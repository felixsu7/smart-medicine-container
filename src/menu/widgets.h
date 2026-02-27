#include "../ui.h"
#include "menu.h"
#include "time.h"
#include LVGL_INCLUDE

static lv_obj_t* create_text(lv_obj_t* parent, const char* icon,
                             const char* txt);

static lv_obj_t* new_page(lv_obj_t* menu, lv_obj_t* menu_section,
                          const char* name) {
  lv_obj_t* page = lv_menu_page_create(menu, NULL);
  lv_obj_set_style_pad_hor(
      page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  lv_menu_separator_create(page);
  lv_menu_section_create(page);

  lv_obj_t* cont = create_text(menu_section, NULL, name);
  lv_menu_set_load_page_event(menu, cont, page);

  return page;
}

static void back_event_handler(lv_event_t* e);
static void switch_handler(lv_event_t* e);
static lv_obj_t* root_page;
static lv_obj_t* create_text(lv_obj_t* parent, const char* icon,
                             const char* txt);
static lv_obj_t* create_slider(lv_obj_t* parent, const char* icon,
                               const char* txt, int32_t min, int32_t max,
                               int32_t val);
static lv_obj_t* create_switch(lv_obj_t* parent, const char* icon,
                               const char* txt, bool chk);

static void motor_cb(lv_observer_t* observer, lv_subject_t* subject) {
  int v = lv_subject_get_int(subject);
  smc_motor_move(v);
}

// static void event_handler(lv_event_t* e) {
//   lv_event_code_t code = lv_event_get_code(e);
//   lv_obj_t* obj = lv_event_get_target_obj(e);
//   if (code == LV_EVENT_CLICKED) {
//     void* idx = (unsigned long*)lv_obj_get_user_data(obj);
//     Alarms* alarms = smc_system_alarms();
//     alarms->attend_idx((unsigned long)idx, smc_time_get(), 0x00);
//   }
// }

// static void alarms_page_refresh(lv_obj_t* page) {
//   Alarms* alarms = smc_system_alarms();
//
//   lv_obj_clean(page);
//
//   lv_obj_t* list = lv_list_create(page);
//
//   for (static int i = MAX_ALARMS - 1; i >= 0; i--) {
//     Alarm alarm;
//     if (alarms->get(i, &alarm) != 0) {
//       continue;
//     }
//
//     char name[40];
//     snprintf(name, sizeof(name), "%s (%d)", alarm.name, alarm.compartment);
//
//     auto btn = lv_list_add_button(list, NULL, name);
//     lv_obj_set_user_data(btn, (unsigned long*)i);
//     lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, NULL);
//   }
// }

static lv_obj_t* create_text(lv_obj_t* parent, const char* icon,
                             const char* txt) {
  lv_obj_t* obj = lv_menu_cont_create(parent);

  lv_obj_t* img = NULL;
  lv_obj_t* label = NULL;

  if (icon) {
    img = lv_image_create(obj);
    lv_image_set_src(img, icon);
  }

  if (txt) {
    label = lv_label_create(obj);
    lv_label_set_text(label, txt);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_obj_set_flex_grow(label, 1);
  }

  return obj;
}

static lv_obj_t* create_slider(lv_obj_t* parent, const char* icon,
                               const char* txt, int32_t min, int32_t max,
                               int32_t val, lv_obj_t* dest = NULL) {
  lv_obj_t* obj = create_text(parent, icon, txt);

  lv_obj_t* slider = lv_slider_create(obj);
  lv_obj_set_flex_grow(slider, 1);
  lv_slider_set_range(slider, min, max);
  lv_slider_set_value(slider, val, LV_ANIM_OFF);

  if (icon == NULL) {
    lv_obj_add_flag(slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  }

  if (dest != NULL) {
    dest = obj;
  }

  return slider;
}

static lv_obj_t* create_switch(lv_obj_t* parent, const char* icon,
                               const char* txt, bool chk) {
  lv_obj_t* obj = create_text(parent, icon, txt);

  lv_obj_t* sw = lv_switch_create(obj);
  lv_obj_add_state(sw, chk ? LV_STATE_CHECKED : LV_STATE_DEFAULT);

  return sw;
}

bool bounce_alt(time_t* timekeeper, int bounce_ms = 1000, bool renew = false) {
  long now = smc_time_get();
  if (*timekeeper < now) {
    *timekeeper = now + bounce_ms;
    return true;
  }
  if (renew) {
    *timekeeper = now + bounce_ms;
  }
  return false;
}
