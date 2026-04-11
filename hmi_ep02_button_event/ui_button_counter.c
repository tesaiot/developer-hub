#include "lvgl.h"
#include "ui_button_counter.h"
#include "counter_logic.h"
#include "app_logo.h"

static counter_state_t s_counter_state;
static counter_button_action_t s_up_action;
static counter_button_action_t s_down_action;

void ui_button_counter_create(void)
{
    lv_obj_t *screen = lv_screen_active();

    /* Screen base style for this workshop page. */
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0F172A), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    /* Logo: keep in top-right and scale 0.5x. */
    lv_obj_t *logo = lv_image_create(screen);
    lv_image_set_src(logo, &APP_LOGO);
    lv_image_set_scale(logo, 128); /* 256 = 1.0x, 128 = 0.5x */
    lv_obj_align(logo, LV_ALIGN_TOP_RIGHT, -20, 16);

    /* Header text block. */
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Button Event Counter");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, -90, 92);

    lv_obj_t *subtitle = lv_label_create(screen);
    lv_label_set_text(subtitle, "Count Up / Count Down");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x94A3B8), LV_PART_MAIN);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align_to(subtitle, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 12);

    lv_obj_t *counter_caption = lv_label_create(screen);
    lv_label_set_text(counter_caption, "COUNT");
    lv_obj_set_style_text_color(counter_caption, lv_color_hex(0x38BDF8), LV_PART_MAIN);
    lv_obj_set_style_text_font(counter_caption, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align_to(counter_caption, subtitle, LV_ALIGN_OUT_BOTTOM_MID, 0, 22);

    lv_obj_t *counter_value_label = lv_label_create(screen);
    lv_obj_set_style_text_color(counter_value_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(counter_value_label, &lv_font_montserrat_40, LV_PART_MAIN);
    lv_obj_set_width(counter_value_label, 140);
    lv_obj_set_style_text_align(counter_value_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align_to(counter_value_label, counter_caption, LV_ALIGN_OUT_BOTTOM_MID, 0, 24);

    /* Initialize counter state with output label binding. */
    counter_logic_init(&s_counter_state, counter_value_label, false);

    /* Button row container using flex layout. */
    lv_obj_t *btn_row = lv_obj_create(screen);
    lv_obj_set_size(btn_row, 620, 108);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(btn_row, 20, LV_PART_MAIN);
    lv_obj_set_layout(btn_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(btn_row, LV_ALIGN_BOTTOM_MID, 0, -24);

    lv_obj_t *btn_up = lv_button_create(btn_row);
    lv_obj_set_size(btn_up, 190, 78);
    lv_obj_set_style_pad_all(btn_up, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_up, 30, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_up, lv_color_hex(0x16A34A), LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn_up, lv_color_hex(0x14532D), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_up, 18, LV_PART_MAIN);

    /* Event payload: tap +1, hold-repeat +5. */
    s_up_action.state = &s_counter_state;
    s_up_action.tap_delta = 1;
    s_up_action.hold_repeat_delta = 5;
    lv_obj_add_event_cb(btn_up, counter_logic_button_event_cb, LV_EVENT_PRESSED, &s_up_action);
    lv_obj_add_event_cb(btn_up, counter_logic_button_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, &s_up_action);

    lv_obj_t *lbl_up = lv_label_create(btn_up);
    lv_label_set_text(lbl_up, "UP +1");
    lv_obj_set_style_text_color(lbl_up, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_up, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_width(lbl_up, lv_pct(100));
    lv_obj_set_style_text_align(lbl_up, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(lbl_up, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn_down = lv_button_create(btn_row);
    lv_obj_set_size(btn_down, 190, 78);
    lv_obj_set_style_pad_all(btn_down, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_down, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_down, lv_color_hex(0xEA580C), LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn_down, lv_color_hex(0x7C2D12), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_down, 14, LV_PART_MAIN);

    /* Event payload: tap -1, hold-repeat -5. */
    s_down_action.state = &s_counter_state;
    s_down_action.tap_delta = -1;
    s_down_action.hold_repeat_delta = -5;
    lv_obj_add_event_cb(btn_down, counter_logic_button_event_cb, LV_EVENT_PRESSED, &s_down_action);
    lv_obj_add_event_cb(btn_down, counter_logic_button_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, &s_down_action);

    lv_obj_t *lbl_down = lv_label_create(btn_down);
    lv_label_set_text(lbl_down, "DOWN -1");
    lv_obj_set_style_text_color(lbl_down, lv_color_hex(0xFFF7ED), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_down, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_width(lbl_down, lv_pct(100));
    lv_obj_set_style_text_align(lbl_down, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(lbl_down, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn_reset = lv_button_create(btn_row);
    lv_obj_set_size(btn_reset, 190, 78);
    lv_obj_set_style_pad_all(btn_reset, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_reset, 40, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_reset, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_reset, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn_reset, lv_color_hex(0xCBD5E1), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_reset, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_reset, counter_logic_reset_event_cb, LV_EVENT_PRESSED, &s_counter_state);

    lv_obj_t *lbl_reset = lv_label_create(btn_reset);
    lv_label_set_text(lbl_reset, "RESET");
    lv_obj_set_style_text_color(lbl_reset, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_reset, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_width(lbl_reset, lv_pct(100));
    lv_obj_set_style_text_align(lbl_reset, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(lbl_reset, LV_ALIGN_CENTER, 0, 0);
}
