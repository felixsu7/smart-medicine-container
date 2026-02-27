/**
 * LVGL Home Screen  (C++, 320x240)
 *
 * Layout:
 *   [ small 98x45 ] [ small 98x45 ] [ small 98x45 ]   top row
 *   [  big 151x112  ] [  big 151x112  ]               middle row
 *   [ small 98x45 ] [ small 98x45 ] [ small 98x45 ]   bottom row
 *
 * apps[] order:  [0][1][2] top | [3][4] big | [5][6][7] bottom
 */

#include <cmath>
#include <cstdio>
#include <cstring>
#include "../thirdparty/lvgl/lvgl.h"

/* ─── Screen & layout ─────────────────────────────────────────────── */
#define SCREEN_W 320
#define SCREEN_H 240
#define STATUS_BAR_H 20
#define BTN_GAP 6
#define ROW_GAP 6

#define BTN_SMALL_W 98
#define BTN_SMALL_H 45
#define BTN_BIG_W 151
#define BTN_BIG_H 112

#define TRANSITION_MS 220

static void alarm_list_rebuild(void);

/* ─── Colour helpers ──────────────────────────────────────────────── */
static inline lv_color_t col(uint8_t r, uint8_t g, uint8_t b) {
  return lv_color_make(r, g, b);
}

/* ─── App descriptor ──────────────────────────────────────────────── */
struct AppInfo {
  const char* icon_label;
  const char* name;
  void (*on_open)(lv_obj_t* content);
  void (*on_close)(lv_obj_t* content);
};

/* ─── Forward declarations ────────────────────────────────────────── */
static void open_phone(lv_obj_t* c);
static void close_phone(lv_obj_t* c);
static void open_messages(lv_obj_t* c);
static void close_messages(lv_obj_t* c);
static void open_camera(lv_obj_t* c);
static void close_camera(lv_obj_t* c);
static void open_browser(lv_obj_t* c);
static void close_browser(lv_obj_t* c);
static void open_maps(lv_obj_t* c);
static void close_maps(lv_obj_t* c);
static void open_music(lv_obj_t* c);
static void close_music(lv_obj_t* c);
static void open_alarms(lv_obj_t* c);
static void close_alarms(lv_obj_t* c);
static void open_settings(lv_obj_t* c);
static void close_settings(lv_obj_t* c);

static const AppInfo apps[8] = {
    {"TEL", "Phone", open_phone, close_phone},
    {"MSG", "Messages", open_messages, close_messages},
    {"CAM", "Camera", open_camera, close_camera},
    {"CHR", "Browser", open_browser, close_browser},
    {"MAP", "Maps", open_maps, close_maps},
    {"MUS", "Music", open_music, close_music},
    {"ALM", "Alarms", open_alarms, close_alarms},
    {"SET", "Settings", open_settings, close_settings},
};

/* ═══════════════════════════════════════════════════════════════════
 * FULLSCREEN KEYBOARD OVERLAY
 *
 * Lives on lv_layer_top() so it floats above everything.
 * Call kb_overlay_show(ta) to attach to a textarea.
 * The overlay dismisses itself when the keyboard fires READY or CANCEL.
 * No panel resizing needed — the kb is a separate layer.
 * ═══════════════════════════════════════════════════════════════════ */
static lv_obj_t* s_kb_overlay = nullptr;
static lv_obj_t* s_kb_widget = nullptr;

static void kb_overlay_close(void) {
  if (!s_kb_overlay)
    return;
  lv_obj_del(s_kb_overlay);
  s_kb_overlay = nullptr;
  s_kb_widget = nullptr;
}

static void kb_event_cb(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  /* READY = ✓ key,  CANCEL = ✗ key — both dismiss the keyboard */
  if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL)
    kb_overlay_close();
}

/**
 * Show the fullscreen keyboard attached to `ta`.
 * Safe to call even if already visible — just re-targets the textarea.
 */
static void kb_overlay_show(lv_obj_t* ta) {
  if (!s_kb_overlay) {
    /* Dim background */
    s_kb_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_kb_overlay, SCREEN_W, SCREEN_H);
    lv_obj_set_pos(s_kb_overlay, 0, 0);
    lv_obj_set_style_bg_color(s_kb_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_kb_overlay, LV_OPA_50, 0);
    lv_obj_set_style_border_width(s_kb_overlay, 0, 0);
    lv_obj_set_style_radius(s_kb_overlay, 0, 0);
    lv_obj_set_style_pad_all(s_kb_overlay, 0, 0);
    lv_obj_clear_flag(s_kb_overlay, LV_OBJ_FLAG_SCROLLABLE);
    /* Tap outside keyboard closes it */
    lv_obj_add_flag(s_kb_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(
        s_kb_overlay,
        [](lv_event_t* ev) {
          if (lv_event_get_code(ev) == LV_EVENT_CLICKED)
            kb_overlay_close();
        },
        LV_EVENT_CLICKED, nullptr);

    /* Keyboard — full width, bottom half */
    s_kb_widget = lv_keyboard_create(s_kb_overlay);
    lv_obj_set_size(s_kb_widget, SCREEN_W, 130);
    lv_obj_align(s_kb_widget, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(s_kb_widget, col(20, 20, 40), 0);
    lv_obj_set_style_text_color(s_kb_widget, lv_color_white(), 0);
    /* Prevent taps on the kb itself from closing the overlay */
    lv_obj_add_event_cb(
        s_kb_widget, [](lv_event_t* ev) { lv_event_stop_bubbling(ev); },
        LV_EVENT_CLICKED, nullptr);

    lv_obj_add_event_cb(s_kb_widget, kb_event_cb, LV_EVENT_READY, nullptr);
    lv_obj_add_event_cb(s_kb_widget, kb_event_cb, LV_EVENT_CANCEL, nullptr);
  }
  lv_keyboard_set_textarea(s_kb_widget, ta);
}

/* Attach to any textarea: tap → show kb, kb dismiss → nothing extra needed */
static void ta_clicked_cb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;
  kb_overlay_show((lv_obj_t*)lv_event_get_target(e));
}

/* ═══════════════════════════════════════════════════════════════════
 * MSGBOX
 * ═══════════════════════════════════════════════════════════════════ */
struct MsgBoxCtx {
  lv_obj_t* overlay;
  void (*on_close)(void);
};

static void msgbox_close_cb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;
  auto* ctx = static_cast<MsgBoxCtx*>(lv_event_get_user_data(e));
  if (ctx->on_close)
    ctx->on_close();
  lv_obj_del(ctx->overlay);
  delete ctx;
}

void msgbox_show(const char* header, const char* body, const char* btn_label,
                 void (*on_close)(void)) {
  lv_obj_t* overlay = lv_obj_create(lv_layer_top());
  lv_obj_set_size(overlay, SCREEN_W, SCREEN_H);
  lv_obj_set_pos(overlay, 0, 0);
  lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(overlay, LV_OPA_60, 0);
  lv_obj_set_style_border_width(overlay, 0, 0);
  lv_obj_set_style_radius(overlay, 0, 0);
  lv_obj_set_style_pad_all(overlay, 0, 0);
  lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t* box = lv_obj_create(overlay);
  lv_obj_set_size(box, 260, 160);
  lv_obj_align(box, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(box, col(30, 30, 55), 0);
  lv_obj_set_style_border_color(box, col(100, 100, 160), 0);
  lv_obj_set_style_border_width(box, 1, 0);
  lv_obj_set_style_radius(box, 12, 0);
  lv_obj_set_style_pad_all(box, 12, 0);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* hdr = lv_label_create(box);
  lv_label_set_text(hdr, header);
  lv_obj_set_width(hdr, 236);
  lv_obj_set_style_text_color(hdr, lv_color_white(), 0);
  lv_obj_set_style_text_font(hdr, &lv_font_montserrat_14, 0);
  lv_label_set_long_mode(hdr, LV_LABEL_LONG_WRAP);
  lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 0);

  lv_obj_t* div = lv_obj_create(box);
  lv_obj_set_size(div, 236, 1);
  lv_obj_set_style_bg_color(div, col(100, 100, 160), 0);
  lv_obj_set_style_border_width(div, 0, 0);
  lv_obj_set_style_radius(div, 0, 0);
  lv_obj_align_to(div, hdr, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

  lv_obj_t* txt = lv_label_create(box);
  lv_label_set_text(txt, body);
  lv_obj_set_width(txt, 236);
  lv_obj_set_style_text_color(txt, col(200, 200, 220), 0);
  lv_obj_set_style_text_font(txt, &lv_font_montserrat_12, 0);
  lv_label_set_long_mode(txt, LV_LABEL_LONG_WRAP);
  lv_obj_align_to(txt, div, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);

  lv_obj_t* btn = lv_btn_create(box);
  lv_obj_set_size(btn, 236, 36);
  lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(btn, col(70, 70, 180), 0);
  lv_obj_set_style_bg_color(btn, col(90, 90, 210), LV_STATE_PRESSED);
  lv_obj_set_style_radius(btn, 8, 0);
  lv_obj_set_style_border_width(btn, 0, 0);

  lv_obj_t* blbl = lv_label_create(btn);
  lv_label_set_text(blbl, btn_label);
  lv_obj_set_style_text_color(blbl, lv_color_white(), 0);
  lv_obj_set_style_text_font(blbl, &lv_font_montserrat_12, 0);
  lv_obj_center(blbl);

  auto* ctx = new MsgBoxCtx{overlay, on_close};
  lv_obj_add_event_cb(btn, msgbox_close_cb, LV_EVENT_CLICKED, ctx);
}

/* ═══════════════════════════════════════════════════════════════════
 * ALARM APP
 * ═══════════════════════════════════════════════════════════════════ */
#define MAX_ALARMS 16
#define DAY_COUNT 7
static const char* DAY_NAMES[DAY_COUNT] = {"Su", "Mo", "Tu", "We",
                                           "Th", "Fr", "Sa"};

struct AlarmEntry {
  char name[32];
  char desc[64];
  uint8_t hour;   /* 1-12 */
  uint8_t minute; /* 0-59 */
  bool is_pm;
  bool days[DAY_COUNT];
};

static AlarmEntry alarm_list[MAX_ALARMS];
static int alarm_count = 0;
static lv_obj_t* alarm_list_container = nullptr;

/* ── Clock face canvas ────────────────────────────────────────────── */
#define CLOCK_CANVAS_SIZE 90
#define CLOCK_CX (CLOCK_CANVAS_SIZE / 2)
#define CLOCK_CY (CLOCK_CANVAS_SIZE / 2)
#define CLOCK_R 42      /* outer circle radius   */
#define CLOCK_HAND_H 28 /* hour hand length      */
#define CLOCK_HAND_M 36 /* minute hand length    */

static lv_color_t s_clock_buf[CLOCK_CANVAS_SIZE * CLOCK_CANVAS_SIZE];

struct ClockCtx {
  lv_obj_t* canvas;
  lv_obj_t* slider_h; /* hour   0-11 */
  lv_obj_t* slider_m; /* minute 0-59 */
  lv_obj_t* ampm_btn;
  lv_obj_t* time_label; /* "12:00 AM" display on face */
};

static void clock_redraw(ClockCtx* ctx) {
  int h = (int)lv_slider_get_value(ctx->slider_h); /* 0-11 */
  int m = (int)lv_slider_get_value(ctx->slider_m); /* 0-59 */

  /* ── Begin drawing layer on the canvas ── */
  lv_layer_t layer;
  lv_canvas_init_layer(ctx->canvas, &layer);

  /* ── Fill background ── */
  lv_draw_fill_dsc_t fill_dsc;
  lv_draw_fill_dsc_init(&fill_dsc);
  fill_dsc.color = col(20, 20, 42);
  fill_dsc.opa = LV_OPA_COVER;
  lv_area_t canvas_area = {0, 0, CLOCK_CANVAS_SIZE, CLOCK_CANVAS_SIZE};
  lv_draw_fill(&layer, &fill_dsc, &canvas_area);

  /* ── Outer circle ── */
  lv_draw_arc_dsc_t arc_dsc;
  lv_draw_arc_dsc_init(&arc_dsc);
  arc_dsc.color = col(90, 90, 140);
  arc_dsc.width = 2;
  arc_dsc.opa = LV_OPA_COVER;
  arc_dsc.center.x = CLOCK_CX;
  arc_dsc.center.y = CLOCK_CY;
  arc_dsc.radius = CLOCK_R;
  arc_dsc.start_angle = 0;
  arc_dsc.end_angle = 360;
  lv_draw_arc(&layer, &arc_dsc);

  /* ── Hour tick marks ── */
  lv_draw_line_dsc_t tick_dsc;
  lv_draw_line_dsc_init(&tick_dsc);
  tick_dsc.color = col(120, 120, 170);
  tick_dsc.width = 1;
  tick_dsc.opa = LV_OPA_COVER;
  for (int i = 0; i < 12; i++) {
    float ang = (float)i / 12.0f * 2.0f * (float)M_PI - (float)M_PI / 2.0f;
    tick_dsc.p1.x = (int32_t)(CLOCK_CX + (CLOCK_R - 5) * cosf(ang));
    tick_dsc.p1.y = (int32_t)(CLOCK_CY + (CLOCK_R - 5) * sinf(ang));
    tick_dsc.p2.x = (int32_t)(CLOCK_CX + (CLOCK_R - 1) * cosf(ang));
    tick_dsc.p2.y = (int32_t)(CLOCK_CY + (CLOCK_R - 1) * sinf(ang));
    lv_draw_line(&layer, &tick_dsc);
  }

  /* ── Minute hand (thin, long, green) ── */
  lv_draw_line_dsc_t hand_m_dsc;
  lv_draw_line_dsc_init(&hand_m_dsc);
  hand_m_dsc.color = col(60, 200, 110);
  hand_m_dsc.width = 2;
  hand_m_dsc.opa = LV_OPA_COVER;
  {
    float ang = (float)m / 60.0f * 2.0f * (float)M_PI - (float)M_PI / 2.0f;
    hand_m_dsc.p1.x = CLOCK_CX;
    hand_m_dsc.p1.y = CLOCK_CY;
    hand_m_dsc.p2.x = (int32_t)(CLOCK_CX + CLOCK_HAND_M * cosf(ang));
    hand_m_dsc.p2.y = (int32_t)(CLOCK_CY + CLOCK_HAND_M * sinf(ang));
    lv_draw_line(&layer, &hand_m_dsc);
  }

  /* ── Hour hand (thick, short, blue) ── */
  lv_draw_line_dsc_t hand_h_dsc;
  lv_draw_line_dsc_init(&hand_h_dsc);
  hand_h_dsc.color = col(80, 130, 255);
  hand_h_dsc.width = 3;
  hand_h_dsc.opa = LV_OPA_COVER;
  {
    float ang = ((float)h + (float)m / 60.0f) / 12.0f * 2.0f * (float)M_PI -
                (float)M_PI / 2.0f;
    hand_h_dsc.p1.x = CLOCK_CX;
    hand_h_dsc.p1.y = CLOCK_CY;
    hand_h_dsc.p2.x = (int32_t)(CLOCK_CX + CLOCK_HAND_H * cosf(ang));
    hand_h_dsc.p2.y = (int32_t)(CLOCK_CY + CLOCK_HAND_H * sinf(ang));
    lv_draw_line(&layer, &hand_h_dsc);
  }

  /* ── Centre dot ── */
  lv_draw_arc_dsc_t dot_dsc;
  lv_draw_arc_dsc_init(&dot_dsc);
  dot_dsc.color = lv_color_white();
  dot_dsc.width = 3;
  dot_dsc.opa = LV_OPA_COVER;
  dot_dsc.center.x = CLOCK_CX;
  dot_dsc.center.y = CLOCK_CY;
  dot_dsc.radius = 3;
  dot_dsc.start_angle = 0;
  dot_dsc.end_angle = 360;
  lv_draw_arc(&layer, &dot_dsc);

  /* ── Finish drawing ── */
  lv_canvas_finish_layer(ctx->canvas, &layer);

  /* ── Update time label ── */
  int disp_h = h == 0 ? 12 : h; /* 0-indexed slider, display 1-12 */
  const char* ampm = lv_label_get_text(lv_obj_get_child(ctx->ampm_btn, 0));
  char buf[12];
  snprintf(buf, sizeof(buf), "%d:%02d %s", disp_h, m, ampm);
  lv_label_set_text(ctx->time_label, buf);
}

static void clock_slider_cb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED)
    return;
  clock_redraw(static_cast<ClockCtx*>(lv_event_get_user_data(e)));
}

static void clock_ampm_cb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;
  auto* ctx = static_cast<ClockCtx*>(lv_event_get_user_data(e));
  lv_obj_t* lbl = lv_obj_get_child(ctx->ampm_btn, 0);
  if (strcmp(lv_label_get_text(lbl), "AM") == 0) {
    lv_label_set_text(lbl, "PM");
    lv_obj_set_style_bg_color(ctx->ampm_btn, col(140, 40, 80),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  } else {
    lv_label_set_text(lbl, "AM");
    lv_obj_set_style_bg_color(ctx->ampm_btn, col(60, 40, 130),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  clock_redraw(ctx);
}

/**
 * Creates the clock widget inside `parent`.
 * Returns a heap-allocated ClockCtx — caller must delete when overlay closes.
 *
 * Layout (all within parent):
 *   Canvas (90x90) — centred top
 *   Time label      — below canvas
 *   H slider row    — below label
 *   M slider row    — below H row
 *   AM/PM button    — below M row, right-aligned
 */
static ClockCtx* clock_widget_create(lv_obj_t* parent, int parent_w) {
  auto* ctx = new ClockCtx{};

  /* Canvas */
  lv_obj_t* canvas = lv_canvas_create(parent);
  lv_canvas_set_buffer(canvas, s_clock_buf, CLOCK_CANVAS_SIZE,
                       CLOCK_CANVAS_SIZE, LV_COLOR_FORMAT_RGB888);
  lv_obj_set_size(canvas, CLOCK_CANVAS_SIZE, CLOCK_CANVAS_SIZE);
  lv_obj_align(canvas, LV_ALIGN_TOP_MID, 0, 0);
  ctx->canvas = canvas;

  /* Time label below canvas */
  lv_obj_t* tlbl = lv_label_create(parent);
  lv_label_set_text(tlbl, "12:00 AM");
  lv_obj_set_style_text_color(tlbl, lv_color_white(), 0);
  lv_obj_set_style_text_font(tlbl, &lv_font_montserrat_12, 0);
  lv_obj_align(tlbl, LV_ALIGN_TOP_MID, 0, CLOCK_CANVAS_SIZE + 4);
  ctx->time_label = tlbl;

  int slider_y = CLOCK_CANVAS_SIZE + 20;
  int slider_w = parent_w - 36; /* room for "H " and "M " labels */
  int lbl_x_h = 0;
  int slider_x = 22;

  /* Hour slider */
  lv_obj_t* h_lbl = lv_label_create(parent);
  lv_label_set_text(h_lbl, "H");
  lv_obj_set_style_text_color(h_lbl, col(80, 130, 255), 0);
  lv_obj_set_style_text_font(h_lbl, &lv_font_montserrat_12, 0);
  lv_obj_set_pos(h_lbl, lbl_x_h, slider_y + 2);

  lv_obj_t* sl_h = lv_slider_create(parent);
  lv_slider_set_range(sl_h, 0, 11);
  lv_slider_set_value(sl_h, 11, LV_ANIM_OFF); /* default 12 o'clock */
  lv_obj_set_size(sl_h, slider_w, 18);
  lv_obj_set_pos(sl_h, slider_x, slider_y);
  lv_obj_set_style_bg_color(sl_h, col(50, 50, 80), LV_PART_MAIN);
  lv_obj_set_style_bg_color(sl_h, col(80, 130, 255), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(sl_h, lv_color_white(), LV_PART_KNOB);
  lv_obj_set_style_radius(sl_h, 4, LV_PART_MAIN);
  ctx->slider_h = sl_h;

  /* Minute slider */
  int m_slider_y = slider_y + 26;
  lv_obj_t* m_lbl = lv_label_create(parent);
  lv_label_set_text(m_lbl, "M");
  lv_obj_set_style_text_color(m_lbl, col(60, 200, 110), 0);
  lv_obj_set_style_text_font(m_lbl, &lv_font_montserrat_12, 0);
  lv_obj_set_pos(m_lbl, lbl_x_h, m_slider_y + 2);

  lv_obj_t* sl_m = lv_slider_create(parent);
  lv_slider_set_range(sl_m, 0, 59);
  lv_slider_set_value(sl_m, 0, LV_ANIM_OFF);
  lv_obj_set_size(sl_m, slider_w, 18);
  lv_obj_set_pos(sl_m, slider_x, m_slider_y);
  lv_obj_set_style_bg_color(sl_m, col(50, 50, 80), LV_PART_MAIN);
  lv_obj_set_style_bg_color(sl_m, col(60, 200, 110), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(sl_m, lv_color_white(), LV_PART_KNOB);
  lv_obj_set_style_radius(sl_m, 4, LV_PART_MAIN);
  ctx->slider_m = sl_m;

  /* AM/PM toggle */
  int ampm_y = m_slider_y + 26;
  lv_obj_t* ampm = lv_btn_create(parent);
  lv_obj_set_size(ampm, 64, 24);
  lv_obj_set_pos(ampm, parent_w - 64, ampm_y);
  lv_obj_set_style_bg_color(ampm, col(60, 40, 130), 0);
  lv_obj_set_style_radius(ampm, 6, 0);
  lv_obj_set_style_border_width(ampm, 0, 0);
  lv_obj_set_style_pad_all(ampm, 0, 0);
  ctx->ampm_btn = ampm;

  lv_obj_t* ampm_lbl = lv_label_create(ampm);
  lv_label_set_text(ampm_lbl, "AM");
  lv_obj_set_style_text_color(ampm_lbl, lv_color_white(), 0);
  lv_obj_set_style_text_font(ampm_lbl, &lv_font_montserrat_12, 0);
  lv_obj_center(ampm_lbl);

  /* Wire events */
  lv_obj_add_event_cb(sl_h, clock_slider_cb, LV_EVENT_VALUE_CHANGED, ctx);
  lv_obj_add_event_cb(sl_m, clock_slider_cb, LV_EVENT_VALUE_CHANGED, ctx);
  lv_obj_add_event_cb(ampm, clock_ampm_cb, LV_EVENT_CLICKED, ctx);

  /* Initial draw */
  clock_redraw(ctx);

  return ctx;
}

/* ═══════════════════════════════════════════════════════════════════
 * ADD ALARM OVERLAY
 * ═══════════════════════════════════════════════════════════════════ */
struct AddAlarmCtx {
  lv_obj_t* overlay;
  lv_obj_t* ta_name;
  lv_obj_t* ta_desc;
  lv_obj_t* day_btns[DAY_COUNT];
  lv_obj_t* calendar;
  ClockCtx* clock_ctx;
};

static void add_alarm_confirm_cb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;
  auto* ctx = static_cast<AddAlarmCtx*>(lv_event_get_user_data(e));

  if (alarm_count < MAX_ALARMS) {
    AlarmEntry& a = alarm_list[alarm_count];

    strncpy(a.name, lv_textarea_get_text(ctx->ta_name), 31);
    a.name[31] = '\0';
    strncpy(a.desc, lv_textarea_get_text(ctx->ta_desc), 63);
    a.desc[63] = '\0';

    int h_raw = (int)lv_slider_get_value(ctx->clock_ctx->slider_h); /* 0-11 */
    a.hour = (uint8_t)(h_raw == 0 ? 12 : h_raw); /* display as 1-12 */
    a.minute = (uint8_t)lv_slider_get_value(ctx->clock_ctx->slider_m);
    a.is_pm =
        (strcmp(
             lv_label_get_text(lv_obj_get_child(ctx->clock_ctx->ampm_btn, 0)),
             "PM") == 0);

    for (int d = 0; d < DAY_COUNT; d++)
      a.days[d] = lv_obj_has_state(ctx->day_btns[d], LV_STATE_CHECKED);

    alarm_count++;
    alarm_list_rebuild();
  }

  lv_obj_del(ctx->overlay);
  delete ctx->clock_ctx;
  delete ctx;
}

static void add_alarm_cancel_cb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;
  auto* ctx = static_cast<AddAlarmCtx*>(lv_event_get_user_data(e));
  lv_obj_del(ctx->overlay);
  delete ctx->clock_ctx;
  delete ctx;
}

static void alarm_add_overlay_create(lv_obj_t* parent_screen) {
  auto* ctx = new AddAlarmCtx{};

  /* ── Full-screen dim overlay on lv_layer_top so it's above everything ── */
  lv_obj_t* overlay = lv_obj_create(lv_layer_top());
  lv_obj_set_size(overlay, SCREEN_W, SCREEN_H);
  lv_obj_set_pos(overlay, 0, 0);
  lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(overlay, LV_OPA_70, 0);
  lv_obj_set_style_border_width(overlay, 0, 0);
  lv_obj_set_style_radius(overlay, 0, 0);
  lv_obj_set_style_pad_all(overlay, 0, 0);
  lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
  ctx->overlay = overlay;

  /* ── Scrollable panel ── */
  const int PANEL_W = 308;
  const int PANEL_H = 224;
  const int PANEL_PAD = 8;
  const int PANEL_INNER_W = PANEL_W - 2 * PANEL_PAD;

  lv_obj_t* panel = lv_obj_create(overlay);
  lv_obj_set_size(panel, PANEL_W, PANEL_H);
  lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(panel, col(22, 22, 45), 0);
  lv_obj_set_style_border_color(panel, col(90, 90, 150), 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_style_radius(panel, 12, 0);
  lv_obj_set_style_pad_all(panel, PANEL_PAD, 0);
  lv_obj_add_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(panel, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_ACTIVE);
  /* Stop overlay-close from firing when tapping the panel */
  lv_obj_add_event_cb(
      panel, [](lv_event_t* ev) { lv_event_stop_bubbling(ev); },
      LV_EVENT_CLICKED, nullptr);

  /* ── Running Y cursor ── */
  int y = 0;

  /* Title */
  lv_obj_t* title = lv_label_create(panel);
  lv_label_set_text(title, "New Alarm");
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(title, 0, y);
  y += 22;

  /* Helper: small section label */
  auto sec = [&](const char* txt) {
    lv_obj_t* l = lv_label_create(panel);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_color(l, col(150, 150, 200), 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_10, 0);
    lv_obj_set_pos(l, 0, y);
    y += 14;
  };

  /* Helper: styled textarea */
  auto make_ta = [&](const char* placeholder, bool multiline) -> lv_obj_t* {
    lv_obj_t* ta = lv_textarea_create(panel);
    if (!multiline)
      lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, placeholder);
    lv_obj_set_size(ta, PANEL_INNER_W, multiline ? 44 : 28);
    lv_obj_set_pos(ta, 0, y);
    lv_obj_set_style_bg_color(ta, col(35, 35, 60), 0);
    lv_obj_set_style_text_color(ta, lv_color_white(), 0);
    lv_obj_set_style_text_font(ta, &lv_font_montserrat_12, 0);
    lv_obj_set_style_border_color(ta, col(80, 80, 140), 0);
    lv_obj_set_style_border_width(ta, 1, 0);
    lv_obj_set_style_radius(ta, 6, 0);
    /* Tap → fullscreen keyboard */
    lv_obj_add_event_cb(ta, ta_clicked_cb, LV_EVENT_CLICKED, nullptr);
    y += (multiline ? 44 : 28) + 4;
    return ta;
  };

  /* Name */
  sec("Name");
  ctx->ta_name = make_ta("Alarm name...", false);

  /* Description */
  sec("Description");
  ctx->ta_desc = make_ta("Optional...", false);

  /* Time — clock widget */
  sec("Time");
  lv_obj_t* clock_cont = lv_obj_create(panel);
  /* Height: canvas(90) + label(16+4) + H slider(18+26) + M slider(18+26) + ampm(24) = 222... budget 200 */
  const int CLOCK_SECT_H = CLOCK_CANVAS_SIZE + 20 + 26 + 26 + 30;
  lv_obj_set_size(clock_cont, PANEL_INNER_W, CLOCK_SECT_H);
  lv_obj_set_pos(clock_cont, 0, y);
  lv_obj_set_style_bg_opa(clock_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(clock_cont, 0, 0);
  lv_obj_set_style_pad_all(clock_cont, 0, 0);
  lv_obj_clear_flag(clock_cont, LV_OBJ_FLAG_SCROLLABLE);
  ctx->clock_ctx = clock_widget_create(clock_cont, PANEL_INNER_W);
  y += CLOCK_SECT_H + 4;

  /* Days of week */
  sec("Repeat");
  lv_obj_t* days_row = lv_obj_create(panel);
  lv_obj_set_size(days_row, PANEL_INNER_W, 30);
  lv_obj_set_pos(days_row, 0, y);
  lv_obj_set_style_bg_opa(days_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(days_row, 0, 0);
  lv_obj_set_style_pad_all(days_row, 0, 0);
  lv_obj_set_flex_flow(days_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(days_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(days_row, LV_OBJ_FLAG_SCROLLABLE);

  for (int d = 0; d < DAY_COUNT; d++) {
    lv_obj_t* db = lv_btn_create(days_row);
    lv_obj_set_size(db, 36, 26);
    lv_obj_set_style_bg_color(db, col(45, 45, 72), 0);
    lv_obj_set_style_bg_color(db, col(55, 85, 210), LV_STATE_CHECKED);
    lv_obj_set_style_radius(db, 5, 0);
    lv_obj_set_style_border_width(db, 0, 0);
    lv_obj_set_style_pad_all(db, 0, 0);
    lv_obj_add_flag(db, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_t* dl = lv_label_create(db);
    lv_label_set_text(dl, DAY_NAMES[d]);
    lv_obj_set_style_text_color(dl, lv_color_white(), 0);
    lv_obj_set_style_text_font(dl, &lv_font_montserrat_10, 0);
    lv_obj_center(dl);
    ctx->day_btns[d] = db;
  }
  y += 34;

  /* Calendar */
  sec("Date");
  lv_obj_t* cal = lv_calendar_create(panel);
  lv_obj_set_size(cal, PANEL_INNER_W, 148);
  lv_obj_set_pos(cal, 0, y);
  lv_calendar_set_today_date(cal, 2025, 1, 1);
  lv_calendar_set_showed_date(cal, 2025, 1);
  lv_obj_set_style_bg_color(cal, col(28, 28, 50), 0);
  lv_obj_set_style_text_color(cal, lv_color_white(), 0);
  lv_calendar_header_arrow_create(cal);
  ctx->calendar = cal;
  y += 152;

  /* Confirm / Cancel */
  int half = (PANEL_INNER_W - 6) / 2;

  lv_obj_t* confirm = lv_btn_create(panel);
  lv_obj_set_size(confirm, half, 34);
  lv_obj_set_pos(confirm, 0, y);
  lv_obj_set_style_bg_color(confirm, col(45, 160, 75), 0);
  lv_obj_set_style_bg_color(confirm, col(65, 190, 95), LV_STATE_PRESSED);
  lv_obj_set_style_radius(confirm, 8, 0);
  lv_obj_set_style_border_width(confirm, 0, 0);
  lv_obj_add_event_cb(confirm, add_alarm_confirm_cb, LV_EVENT_CLICKED, ctx);

  lv_obj_t* clbl = lv_label_create(confirm);
  lv_label_set_text(clbl, LV_SYMBOL_OK "  Add");
  lv_obj_set_style_text_color(clbl, lv_color_white(), 0);
  lv_obj_set_style_text_font(clbl, &lv_font_montserrat_12, 0);
  lv_obj_center(clbl);

  lv_obj_t* cancel = lv_btn_create(panel);
  lv_obj_set_size(cancel, half, 34);
  lv_obj_set_pos(cancel, half + 6, y);
  lv_obj_set_style_bg_color(cancel, col(160, 45, 45), 0);
  lv_obj_set_style_bg_color(cancel, col(190, 65, 65), LV_STATE_PRESSED);
  lv_obj_set_style_radius(cancel, 8, 0);
  lv_obj_set_style_border_width(cancel, 0, 0);
  lv_obj_add_event_cb(cancel, add_alarm_cancel_cb, LV_EVENT_CLICKED, ctx);

  lv_obj_t* xlbl = lv_label_create(cancel);
  lv_label_set_text(xlbl, LV_SYMBOL_CLOSE "  Cancel");
  lv_obj_set_style_text_color(xlbl, lv_color_white(), 0);
  lv_obj_set_style_text_font(xlbl, &lv_font_montserrat_12, 0);
  lv_obj_center(xlbl);
}

/* ─── Alarm list rebuild ──────────────────────────────────────────── */
static void alarm_list_rebuild(void) {
  if (!alarm_list_container)
    return;
  lv_obj_clean(alarm_list_container);

  if (alarm_count == 0) {
    lv_obj_t* empty = lv_label_create(alarm_list_container);
    lv_label_set_text(empty, "No alarms.\nTap  +  to add one.");
    lv_obj_set_style_text_color(empty, col(130, 130, 165), 0);
    lv_obj_set_style_text_font(empty, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_align(empty, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(empty);
    return;
  }

  for (int i = 0; i < alarm_count; i++) {
    const AlarmEntry& a = alarm_list[i];

    lv_obj_t* row = lv_obj_create(alarm_list_container);
    lv_obj_set_size(row, lv_pct(100), 50);
    lv_obj_set_style_bg_color(row, col(35, 35, 60), 0);
    lv_obj_set_style_radius(row, 8, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 6, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    /* Icon */
    lv_obj_t* icon = lv_label_create(row);
    lv_label_set_text(icon, LV_SYMBOL_BELL);
    lv_obj_set_style_text_color(icon, col(100, 180, 255), 0);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, 0);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 2, 0);

    /* Name */
    lv_obj_t* name_l = lv_label_create(row);
    lv_label_set_text(name_l, a.name[0] ? a.name : "(unnamed)");
    lv_obj_set_style_text_color(name_l, lv_color_white(), 0);
    lv_obj_set_style_text_font(name_l, &lv_font_montserrat_12, 0);
    lv_label_set_long_mode(name_l, LV_LABEL_LONG_CLIP);
    lv_obj_set_size(name_l, 140, 16);
    lv_obj_align(name_l, LV_ALIGN_TOP_LEFT, 26, 4);

    /* Description */
    if (a.desc[0]) {
      lv_obj_t* desc_l = lv_label_create(row);
      lv_label_set_text(desc_l, a.desc);
      lv_obj_set_style_text_color(desc_l, col(155, 155, 185), 0);
      lv_obj_set_style_text_font(desc_l, &lv_font_montserrat_10, 0);
      lv_label_set_long_mode(desc_l, LV_LABEL_LONG_CLIP);
      lv_obj_set_size(desc_l, 140, 14);
      lv_obj_align(desc_l, LV_ALIGN_TOP_LEFT, 26, 22);
    }

    /* Days */
    char day_str[20] = {};
    bool any = false;
    for (int d = 0; d < DAY_COUNT; d++) {
      if (a.days[d]) {
        if (any)
          strcat(day_str, " ");
        strcat(day_str, DAY_NAMES[d]);
        any = true;
      }
    }
    if (!any)
      strcpy(day_str, "Once");

    lv_obj_t* days_l = lv_label_create(row);
    lv_label_set_text(days_l, day_str);
    lv_obj_set_style_text_color(days_l, col(115, 115, 155), 0);
    lv_obj_set_style_text_font(days_l, &lv_font_montserrat_10, 0);
    lv_obj_align(days_l, LV_ALIGN_BOTTOM_LEFT, 26, -4);

    /* Time */
    char time_str[12];
    snprintf(time_str, sizeof(time_str), "%d:%02d %s", a.hour, a.minute,
             a.is_pm ? "PM" : "AM");
    lv_obj_t* time_l = lv_label_create(row);
    lv_label_set_text(time_l, time_str);
    lv_obj_set_style_text_color(time_l, col(80, 210, 140), 0);
    lv_obj_set_style_text_font(time_l, &lv_font_montserrat_14, 0);
    lv_obj_align(time_l, LV_ALIGN_RIGHT_MID, -4, 0);
  }
}

/* ─── Add button callback ─────────────────────────────────────────── */
static void alarm_add_btn_cb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;
  /* parent_screen not needed — overlay goes on lv_layer_top() */
  alarm_add_overlay_create(nullptr);
}

/* ─── Alarm open/close ────────────────────────────────────────────── */
static void open_alarms(lv_obj_t* content) {
  lv_obj_set_style_pad_all(content, 0, 0);

  /* Header bar */
  lv_obj_t* hdr = lv_obj_create(content);
  lv_obj_set_size(hdr, lv_pct(100), 30);
  lv_obj_align(hdr, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_bg_color(hdr, col(25, 25, 50), 0);
  lv_obj_set_style_border_width(hdr, 0, 0);
  lv_obj_set_style_radius(hdr, 0, 0);
  lv_obj_set_style_pad_all(hdr, 4, 0);
  lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* hdr_lbl = lv_label_create(hdr);
  lv_label_set_text(hdr_lbl, LV_SYMBOL_BELL "  Alarms");
  lv_obj_set_style_text_color(hdr_lbl, lv_color_white(), 0);
  lv_obj_set_style_text_font(hdr_lbl, &lv_font_montserrat_12, 0);
  lv_obj_align(hdr_lbl, LV_ALIGN_LEFT_MID, 4, 0);

  lv_obj_t* add_btn = lv_btn_create(hdr);
  lv_obj_set_size(add_btn, 52, 22);
  lv_obj_align(add_btn, LV_ALIGN_RIGHT_MID, -2, 0);
  lv_obj_set_style_bg_color(add_btn, col(45, 160, 75), 0);
  lv_obj_set_style_bg_color(add_btn, col(65, 190, 95), LV_STATE_PRESSED);
  lv_obj_set_style_radius(add_btn, 6, 0);
  lv_obj_set_style_border_width(add_btn, 0, 0);
  lv_obj_set_style_pad_all(add_btn, 0, 0);

  lv_obj_t* add_lbl = lv_label_create(add_btn);
  lv_label_set_text(add_lbl, LV_SYMBOL_PLUS " Add");
  lv_obj_set_style_text_color(add_lbl, lv_color_white(), 0);
  lv_obj_set_style_text_font(add_lbl, &lv_font_montserrat_12, 0);
  lv_obj_center(add_lbl);

  lv_obj_add_event_cb(add_btn, alarm_add_btn_cb, LV_EVENT_CLICKED, nullptr);

  /* Scrollable list */
  lv_obj_t* list = lv_obj_create(content);
  lv_obj_set_size(list, lv_pct(100), lv_obj_get_height(content) - 30);
  lv_obj_align(list, LV_ALIGN_TOP_LEFT, 0, 30);
  lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(list, 0, 0);
  lv_obj_set_style_pad_all(list, 2, 0);
  lv_obj_set_style_pad_row(list, 4, 0);
  lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
  lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(list, LV_DIR_VER);

  alarm_list_container = list;
  alarm_list_rebuild();
}

static void close_alarms(lv_obj_t*) {
  alarm_list_container = nullptr;
  LV_LOG_USER("Alarms closed");
}

/* ═══════════════════════════════════════════════════════════════════
 * NAVIGATION
 * ═══════════════════════════════════════════════════════════════════ */
struct AppScreenCtx {
  const AppInfo* app;
  lv_obj_t* home_screen;
};

static void back_btn_event_cb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;
  auto* ctx = static_cast<AppScreenCtx*>(lv_event_get_user_data(e));
  lv_obj_t* content = lv_obj_get_child(lv_scr_act(), 1);
  if (ctx->app->on_close)
    ctx->app->on_close(content);
  lv_scr_load_anim(ctx->home_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, TRANSITION_MS,
                   0, false);
}

static void open_app_screen(const AppInfo* app, lv_obj_t* home_screen) {
  lv_obj_t* app_scr = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(app_scr, col(15, 15, 30), 0);
  lv_obj_clear_flag(app_scr, LV_OBJ_FLAG_SCROLLABLE);

  const int TOP_BAR_H = 28;
  lv_obj_t* top_bar = lv_obj_create(app_scr);
  lv_obj_set_size(top_bar, SCREEN_W, TOP_BAR_H);
  lv_obj_align(top_bar, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_bg_color(top_bar, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(top_bar, LV_OPA_50, 0);
  lv_obj_set_style_border_width(top_bar, 0, 0);
  lv_obj_set_style_radius(top_bar, 0, 0);
  lv_obj_set_style_pad_all(top_bar, 4, 0);
  lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* back_btn = lv_btn_create(top_bar);
  lv_obj_set_size(back_btn, 60, TOP_BAR_H - 6);
  lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_set_style_bg_color(back_btn, col(70, 70, 120), 0);
  lv_obj_set_style_radius(back_btn, 6, 0);
  lv_obj_set_style_border_width(back_btn, 0, 0);
  lv_obj_set_style_pad_all(back_btn, 0, 0);

  lv_obj_t* back_lbl = lv_label_create(back_btn);
  lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " Back");
  lv_obj_set_style_text_color(back_lbl, lv_color_white(), 0);
  lv_obj_set_style_text_font(back_lbl, &lv_font_montserrat_10, 0);
  lv_obj_center(back_lbl);

  lv_obj_t* title = lv_label_create(top_bar);
  lv_label_set_text(title, app->name);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

  auto* ctx = new AppScreenCtx{app, home_screen};
  lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, ctx);
  lv_obj_add_event_cb(
      app_scr,
      [](lv_event_t* ev) {
        delete static_cast<AppScreenCtx*>(lv_event_get_user_data(ev));
      },
      LV_EVENT_DELETE, ctx);

  lv_obj_t* content = lv_obj_create(app_scr);
  lv_obj_set_size(content, SCREEN_W, SCREEN_H - TOP_BAR_H);
  lv_obj_align(content, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(content, 0, 0);
  lv_obj_set_style_pad_all(content, 0, 0);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

  if (app->on_open)
    app->on_open(content);

  lv_scr_load_anim(app_scr, LV_SCR_LOAD_ANIM_MOVE_LEFT, TRANSITION_MS, 0,
                   false);
}

static void home_btn_event_cb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;
  open_app_screen(static_cast<const AppInfo*>(lv_event_get_user_data(e)),
                  lv_scr_act());
}

static void btn_press_feedback_cb(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
  if (code == LV_EVENT_PRESSED)
    lv_obj_set_style_opa(btn, LV_OPA_70, 0);
  else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    lv_obj_set_style_opa(btn, LV_OPA_COVER, 0);
}

static lv_obj_t* create_btn(lv_obj_t* parent, const AppInfo* app, int x, int y,
                            int w, int h) {
  lv_obj_t* btn = lv_obj_create(parent);
  lv_obj_set_size(btn, w, h);
  lv_obj_set_pos(btn, x, y);
  lv_obj_set_style_bg_color(btn, col(55, 55, 85), 0);
  lv_obj_set_style_radius(btn, 10, 0);
  lv_obj_set_style_border_width(btn, 0, 0);
  lv_obj_set_style_shadow_width(btn, 6, 0);
  lv_obj_set_style_shadow_opa(btn, LV_OPA_30, 0);
  lv_obj_set_style_pad_all(btn, 0, 0);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* icon_lbl = lv_label_create(btn);
  lv_label_set_text(icon_lbl, app->icon_label);
  lv_obj_set_style_text_color(icon_lbl, lv_color_white(), 0);
  lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_14, 0);
  lv_obj_align(icon_lbl, LV_ALIGN_CENTER, 0, -8);

  lv_obj_t* name_lbl = lv_label_create(btn);
  lv_label_set_text(name_lbl, app->name);
  lv_obj_set_style_text_color(name_lbl, col(190, 190, 210), 0);
  lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_10, 0);
  lv_label_set_long_mode(name_lbl, LV_LABEL_LONG_CLIP);
  lv_obj_set_width(name_lbl, w - 6);
  lv_obj_set_style_text_align(name_lbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(name_lbl, LV_ALIGN_BOTTOM_MID, 0, -3);

  lv_obj_add_event_cb(btn, home_btn_event_cb, LV_EVENT_CLICKED, (void*)app);
  lv_obj_add_event_cb(btn, btn_press_feedback_cb, LV_EVENT_PRESSED, nullptr);
  lv_obj_add_event_cb(btn, btn_press_feedback_cb, LV_EVENT_RELEASED, nullptr);
  lv_obj_add_event_cb(btn, btn_press_feedback_cb, LV_EVENT_PRESS_LOST, nullptr);
  return btn;
}

static void create_status_bar(lv_obj_t* scr) {
  lv_obj_t* bar = lv_obj_create(scr);
  lv_obj_set_size(bar, SCREEN_W, STATUS_BAR_H);
  lv_obj_align(bar, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_bg_color(bar, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(bar, LV_OPA_30, 0);
  lv_obj_set_style_border_width(bar, 0, 0);
  lv_obj_set_style_radius(bar, 0, 0);
  lv_obj_set_style_pad_hor(bar, 6, 0);
  lv_obj_set_style_pad_ver(bar, 2, 0);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* time_lbl = lv_label_create(bar);
  lv_label_set_text(time_lbl, "9:41");
  lv_obj_set_style_text_color(time_lbl, lv_color_white(), 0);
  lv_obj_set_style_text_font(time_lbl, &lv_font_montserrat_12, 0);
  lv_obj_align(time_lbl, LV_ALIGN_LEFT_MID, 0, 0);

  lv_obj_t* status_r = lv_label_create(bar);
  lv_label_set_text(status_r, "WiFi  100%");
  lv_obj_set_style_text_color(status_r, lv_color_white(), 0);
  lv_obj_set_style_text_font(status_r, &lv_font_montserrat_10, 0);
  lv_obj_align(status_r, LV_ALIGN_RIGHT_MID, 0, 0);
}

/* ═══════════════════════════════════════════════════════════════════
 * HOME SCREEN
 * ═══════════════════════════════════════════════════════════════════ */
void homescreen_create(void) {
  lv_obj_t* scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, col(20, 20, 40), 0);
  lv_obj_set_style_bg_grad_color(scr, col(55, 18, 75), 0);
  lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  create_status_bar(scr);

  /* Row Y positions — fills 320x240 with zero overflow:
     *   row0  26..71    (45px)
     *   row1  77..189   (112px)
     *   row2  195..240  (45px)  ← bottom == 240 exactly
     */
  const int row0_y = STATUS_BAR_H + ROW_GAP;         /* 26  */
  const int row1_y = row0_y + BTN_SMALL_H + ROW_GAP; /* 77  */
  const int row2_y = row1_y + BTN_BIG_H + ROW_GAP;   /* 195 */

  auto place_row = [&](int start_idx, int count, int btn_w, int btn_h, int y) {
    int tw = count * btn_w + (count - 1) * BTN_GAP;
    int sx = (SCREEN_W - tw) / 2;
    for (int i = 0; i < count; i++)
      create_btn(scr, &apps[start_idx + i], sx + i * (btn_w + BTN_GAP), y,
                 btn_w, btn_h);
  };

  place_row(0, 3, BTN_SMALL_W, BTN_SMALL_H, row0_y);
  place_row(3, 2, BTN_BIG_W, BTN_BIG_H, row1_y);
  place_row(5, 3, BTN_SMALL_W, BTN_SMALL_H, row2_y);
}

/* ═══════════════════════════════════════════════════════════════════
 * STUB APPS
 * ═══════════════════════════════════════════════════════════════════ */
static void stub_screen(lv_obj_t* content, const char* text) {
  lv_obj_t* l = lv_label_create(content);
  lv_label_set_text(l, text);
  lv_obj_set_style_text_color(l, col(200, 200, 220), 0);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_12, 0);
  lv_obj_center(l);
}

static void open_phone(lv_obj_t* c) {
  stub_screen(c, "Phone");
}

static void open_messages(lv_obj_t* c) {
  stub_screen(c, "Messages");
}

static void open_camera(lv_obj_t* c) {
  stub_screen(c, "Camera");
}

static void open_browser(lv_obj_t* c) {
  stub_screen(c, "Browser");
}

static void open_maps(lv_obj_t* c) {
  stub_screen(c, "Maps");
}

static void open_music(lv_obj_t* c) {
  stub_screen(c, "Music");
}

static void open_settings(lv_obj_t* c) {
  stub_screen(c, "Settings");
}

static void close_phone(lv_obj_t*) {
  LV_LOG_USER("Phone closed");
}

static void close_messages(lv_obj_t*) {
  LV_LOG_USER("Messages closed");
}

static void close_camera(lv_obj_t*) {
  LV_LOG_USER("Camera closed");
}

static void close_browser(lv_obj_t*) {
  LV_LOG_USER("Browser closed");
}

static void close_maps(lv_obj_t*) {
  LV_LOG_USER("Maps closed");
}

static void close_music(lv_obj_t*) {
  LV_LOG_USER("Music closed");
}

static void close_settings(lv_obj_t*) {
  LV_LOG_USER("Settings closed");
}
