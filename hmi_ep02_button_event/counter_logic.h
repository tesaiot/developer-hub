#ifndef COUNTER_LOGIC_H
#define COUNTER_LOGIC_H

#include <stdbool.h>
#include <stdint.h>
#include "lvgl.h"

typedef struct {
    int32_t value;
    lv_obj_t *value_label;
    bool allow_negative;
} counter_state_t;

typedef struct {
    counter_state_t *state;
    int32_t tap_delta;
    int32_t hold_repeat_delta;
} counter_button_action_t;

void counter_logic_init(counter_state_t *state, lv_obj_t *value_label, bool allow_negative);
void counter_logic_refresh_label(counter_state_t *state);
void counter_logic_button_event_cb(lv_event_t *e);
void counter_logic_reset_event_cb(lv_event_t *e);

#endif /* COUNTER_LOGIC_H */
