#ifndef TEXT_INPUT_LOGIC_H
#define TEXT_INPUT_LOGIC_H

#include "lvgl.h"

typedef struct {
    /* Input A: updates output immediately while typing. */
    lv_obj_t *realtime_textarea;
    /* Input B: updates output only when keyboard READY is pressed. */
    lv_obj_t *confirmed_textarea;

    lv_obj_t *realtime_label;
    lv_obj_t *confirmed_label;

    lv_obj_t *keyboard;
    lv_obj_t *mode_dropdown;

    /* Tracks which textarea currently owns the keyboard. */
    lv_obj_t *active_textarea;
} text_input_state_t;

void text_input_logic_init(text_input_state_t *state,
                           lv_obj_t *realtime_textarea,
                           lv_obj_t *confirmed_textarea,
                           lv_obj_t *realtime_label,
                           lv_obj_t *confirmed_label,
                           lv_obj_t *keyboard,
                           lv_obj_t *mode_dropdown);

void text_input_logic_textarea_event_cb(lv_event_t *e);
void text_input_logic_keyboard_event_cb(lv_event_t *e);
void text_input_logic_mode_event_cb(lv_event_t *e);

#endif /* TEXT_INPUT_LOGIC_H */
