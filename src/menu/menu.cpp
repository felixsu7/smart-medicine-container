#include "menu.h"
#include "../ui.h"
#include "./boot_logo.h"
#include LVGL_INCLUDE

lv_subject_t motor_subject;

typedef enum {
  LV_MENU_ITEM_BUILDER_VARIANT_1,
  LV_MENU_ITEM_BUILDER_VARIANT_2
} lv_menu_builder_variant_t;

static void back_event_handler(lv_event_t* e);
static void switch_handler(lv_event_t* e);
static lv_obj_t* root_page;
static lv_obj_t* create_text(lv_obj_t* parent, const char* icon,
                             const char* txt,
                             lv_menu_builder_variant_t builder_variant);
static lv_obj_t* create_slider(lv_obj_t* parent, const char* icon,
                               const char* txt, int32_t min, int32_t max,
                               int32_t val);
static lv_obj_t* create_switch(lv_obj_t* parent, const char* icon,
                               const char* txt, bool chk);

static void motor_cb(lv_observer_t* observer, lv_subject_t* subject) {
  int v = lv_subject_get_int(subject);
  smc_motor_move(v);
}

static lv_obj_t* create_text(lv_obj_t* parent, const char* icon,
                             const char* txt,
                             lv_menu_builder_variant_t builder_variant) {
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

  if (builder_variant == LV_MENU_ITEM_BUILDER_VARIANT_2 && icon && txt) {
    lv_obj_add_flag(img, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    lv_obj_swap(img, label);
  }

  return obj;
}

static lv_obj_t* create_slider(lv_obj_t* parent, const char* icon,
                               const char* txt, int32_t min, int32_t max,
                               int32_t val) {
  lv_obj_t* obj =
      create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_2);

  lv_obj_t* slider = lv_slider_create(obj);
  lv_obj_set_flex_grow(slider, 1);
  lv_slider_set_range(slider, min, max);
  lv_slider_set_value(slider, val, LV_ANIM_OFF);

  if (icon == NULL) {
    lv_obj_add_flag(slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  }

  return slider;
}

static lv_obj_t* create_switch(lv_obj_t* parent, const char* icon,
                               const char* txt, bool chk) {
  lv_obj_t* obj =
      create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_1);

  lv_obj_t* sw = lv_switch_create(obj);
  lv_obj_add_state(sw, chk ? LV_STATE_CHECKED : LV_STATE_DEFAULT);

  return sw;
}

void test_menu(void) {
  lv_obj_t* menu = lv_menu_create(lv_screen_active());

  lv_color_t bg_color = lv_obj_get_style_bg_color(menu, LV_PART_MAIN);
  if (lv_color_brightness(bg_color) > 127) {
    lv_obj_set_style_bg_color(
        menu,
        lv_color_darken(lv_obj_get_style_bg_color(menu, LV_PART_MAIN), 10), 0);
  } else {
    lv_obj_set_style_bg_color(
        menu,
        lv_color_darken(lv_obj_get_style_bg_color(menu, LV_PART_MAIN), 50), 0);
  }
  lv_obj_set_size(menu, lv_display_get_horizontal_resolution(NULL),
                  lv_display_get_vertical_resolution(NULL));
  lv_obj_center(menu);

  lv_obj_t* cont;
  lv_obj_t* section;

  /*Create sub pages*/
  lv_obj_t* motor_page = lv_menu_page_create(menu, NULL);
  lv_obj_set_style_pad_hor(
      motor_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  lv_menu_separator_create(motor_page);
  section = lv_menu_section_create(motor_page);
  auto motor_slider =
      create_slider(section, LV_SYMBOL_SETTINGS, "Compartment", 0, 11, 0);

  lv_subject_init_int(&motor_subject, 0);
  lv_slider_bind_value(motor_slider, &motor_subject);
  lv_subject_add_observer(&motor_subject, motor_cb, NULL);

  lv_obj_t* sub_software_info_page = lv_menu_page_create(menu, NULL);
  lv_obj_set_style_pad_hor(
      sub_software_info_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  section = lv_menu_section_create(sub_software_info_page);
  create_text(section, NULL, smc_general_sw_info(),
              LV_MENU_ITEM_BUILDER_VARIANT_1);

  lv_obj_t* sub_about_page = lv_menu_page_create(menu, NULL);
  lv_obj_set_style_pad_hor(
      sub_about_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  lv_menu_separator_create(sub_about_page);
  section = lv_menu_section_create(sub_about_page);
  cont = create_text(section, NULL, "Software information",
                     LV_MENU_ITEM_BUILDER_VARIANT_1);
  lv_menu_set_load_page_event(menu, cont, sub_software_info_page);

  /*Create a root page*/
  root_page = lv_menu_page_create(menu, "smcOS");
  lv_obj_set_style_pad_hor(
      root_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  section = lv_menu_section_create(root_page);
  cont = create_text(section, LV_SYMBOL_SETTINGS, "Motor",
                     LV_MENU_ITEM_BUILDER_VARIANT_1);
  lv_menu_set_load_page_event(menu, cont, motor_page);
  cont = create_text(section, NULL, "About", LV_MENU_ITEM_BUILDER_VARIANT_1);
  lv_menu_set_load_page_event(menu, cont, sub_about_page);

  lv_menu_set_sidebar_page(menu, root_page);

  lv_obj_send_event(
      lv_obj_get_child(lv_obj_get_child(lv_menu_get_cur_sidebar_page(menu), 0),
                       0),
      LV_EVENT_CLICKED, NULL);
}
