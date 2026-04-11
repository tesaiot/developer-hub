#include "lvgl.h"
#include "ui_text_input_keyboard.h"
#include "ui_text_input_layout.h"
#include "text_input_logic.h"
#include "app_logo.h"

static text_input_state_t s_text_input_state;

void ui_text_input_keyboard_create(void)
{
    lv_obj_t *screen = lv_screen_active();

    /* Lock root screen to avoid accidental scroll area when objects exceed bounds. */
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);

    lv_obj_set_style_bg_color(screen, lv_color_hex(UI_TEXT_INPUT_BG_COLOR_HEX), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t *logo = lv_image_create(screen);
    lv_image_set_src(logo, &APP_LOGO);
    lv_image_set_scale(logo, UI_TEXT_INPUT_LOGO_SCALE);
    lv_obj_align(logo, LV_ALIGN_BOTTOM_MID, UI_TEXT_INPUT_LOGO_ALIGN_X, UI_TEXT_INPUT_LOGO_ALIGN_Y);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Text Input Keyboard");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, UI_TEXT_INPUT_LEFT_X, UI_TEXT_INPUT_TITLE_Y);

    lv_obj_t *subtitle = lv_label_create(screen);
    lv_label_set_text(subtitle, "Select mode, then tap an input to open keyboard");
    lv_obj_set_width(subtitle, UI_TEXT_INPUT_SUBTITLE_W);
    lv_label_set_long_mode(subtitle, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x94A3B8), LV_PART_MAIN);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align_to(subtitle, title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, UI_TEXT_INPUT_SUBTITLE_OFF_Y);

    lv_obj_t *mode_label = lv_label_create(screen);
    lv_label_set_text(mode_label, "Keyboard Mode");
    lv_obj_set_style_text_color(mode_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(mode_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(mode_label, LV_ALIGN_TOP_LEFT, UI_TEXT_INPUT_LEFT_X, UI_TEXT_INPUT_MODE_LABEL_Y);

    lv_obj_t *mode_dropdown = lv_dropdown_create(screen);
    lv_dropdown_set_options(mode_dropdown, "Normal\nNumber");
    lv_dropdown_set_selected(mode_dropdown, 0);
    lv_obj_set_size(mode_dropdown, UI_TEXT_INPUT_MODE_DD_W, LV_SIZE_CONTENT);
    lv_obj_align(mode_dropdown,
                 LV_ALIGN_TOP_LEFT,
                 UI_TEXT_INPUT_LEFT_X + UI_TEXT_INPUT_MODE_DD_X,
                 UI_TEXT_INPUT_MODE_DD_Y);

    lv_obj_t *realtime_input_title = lv_label_create(screen);
    lv_label_set_text(realtime_input_title, "Input A (Realtime)");
    lv_obj_set_style_text_color(realtime_input_title, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(realtime_input_title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(realtime_input_title, LV_ALIGN_TOP_LEFT, UI_TEXT_INPUT_LEFT_X, UI_TEXT_INPUT_A_TITLE_Y);

    lv_obj_t *realtime_input = lv_textarea_create(screen);
    lv_obj_set_size(realtime_input, UI_TEXT_INPUT_INPUT_W, UI_TEXT_INPUT_INPUT_H);
    lv_obj_align(realtime_input, LV_ALIGN_TOP_LEFT, UI_TEXT_INPUT_LEFT_X, UI_TEXT_INPUT_A_INPUT_Y);
    lv_textarea_set_one_line(realtime_input, true);
    lv_textarea_set_max_length(realtime_input, 32);

    lv_obj_t *realtime_label = lv_label_create(screen);
    lv_obj_set_width(realtime_label, UI_TEXT_INPUT_OUTPUT_W);
    lv_label_set_long_mode(realtime_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(realtime_label, lv_color_hex(0x7DD3FC), LV_PART_MAIN);
    lv_obj_set_style_text_font(realtime_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(realtime_label, LV_ALIGN_TOP_LEFT, UI_TEXT_INPUT_OUTPUT_X, UI_TEXT_INPUT_A_OUTPUT_Y);

    lv_obj_t *confirmed_input_title = lv_label_create(screen);
    lv_label_set_text(confirmed_input_title, "Input B (Confirmed on OK)");
    lv_obj_set_style_text_color(confirmed_input_title, lv_color_hex(0xC4B5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(confirmed_input_title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(confirmed_input_title, LV_ALIGN_TOP_LEFT, UI_TEXT_INPUT_LEFT_X, UI_TEXT_INPUT_B_TITLE_Y);

    lv_obj_t *confirmed_input = lv_textarea_create(screen);
    lv_obj_set_size(confirmed_input, UI_TEXT_INPUT_INPUT_W, UI_TEXT_INPUT_INPUT_H);
    lv_obj_align(confirmed_input, LV_ALIGN_TOP_LEFT, UI_TEXT_INPUT_LEFT_X, UI_TEXT_INPUT_B_INPUT_Y);
    lv_textarea_set_one_line(confirmed_input, true);
    lv_textarea_set_max_length(confirmed_input, 32);

    lv_obj_t *confirmed_label = lv_label_create(screen);
    lv_obj_set_width(confirmed_label, UI_TEXT_INPUT_OUTPUT_W);
    lv_label_set_long_mode(confirmed_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(confirmed_label, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_text_font(confirmed_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(confirmed_label, LV_ALIGN_TOP_LEFT, UI_TEXT_INPUT_OUTPUT_X, UI_TEXT_INPUT_B_OUTPUT_Y);

    lv_obj_t *hint_label = lv_label_create(screen);
    lv_label_set_text(hint_label, "A updates immediately, B updates when pressing OK");
    lv_obj_set_style_text_color(hint_label, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(hint_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(hint_label, LV_ALIGN_TOP_LEFT, UI_TEXT_INPUT_LEFT_X, UI_TEXT_INPUT_HINT_Y);

    lv_obj_t *keyboard = lv_keyboard_create(screen);
    lv_obj_set_size(keyboard, UI_TEXT_INPUT_KB_W, UI_TEXT_INPUT_KB_H);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, UI_TEXT_INPUT_KB_ALIGN_X, UI_TEXT_INPUT_KB_ALIGN_Y);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);

    text_input_logic_init(&s_text_input_state,
                          realtime_input,
                          confirmed_input,
                          realtime_label,
                          confirmed_label,
                          keyboard,
                          mode_dropdown);

    /* Input A: open keyboard + realtime update while typing. */
    lv_obj_add_event_cb(realtime_input, text_input_logic_textarea_event_cb, LV_EVENT_CLICKED, &s_text_input_state);
    lv_obj_add_event_cb(realtime_input, text_input_logic_textarea_event_cb, LV_EVENT_FOCUSED, &s_text_input_state);
    lv_obj_add_event_cb(realtime_input, text_input_logic_textarea_event_cb, LV_EVENT_VALUE_CHANGED, &s_text_input_state);

    /* Input B: open keyboard; output updates on keyboard READY. */
    lv_obj_add_event_cb(confirmed_input, text_input_logic_textarea_event_cb, LV_EVENT_CLICKED, &s_text_input_state);
    lv_obj_add_event_cb(confirmed_input, text_input_logic_textarea_event_cb, LV_EVENT_FOCUSED, &s_text_input_state);
    lv_obj_add_event_cb(confirmed_input, text_input_logic_textarea_event_cb, LV_EVENT_VALUE_CHANGED, &s_text_input_state);

    lv_obj_add_event_cb(keyboard, text_input_logic_keyboard_event_cb, LV_EVENT_READY, &s_text_input_state);
    lv_obj_add_event_cb(keyboard, text_input_logic_keyboard_event_cb, LV_EVENT_CANCEL, &s_text_input_state);

    /* Dropdown changes mode only (Normal/Number), no direct keyboard popup. */
    lv_obj_add_event_cb(mode_dropdown, text_input_logic_mode_event_cb, LV_EVENT_VALUE_CHANGED, &s_text_input_state);
}
