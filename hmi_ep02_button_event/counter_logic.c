#include "counter_logic.h"
#include <stdio.h>

#define COUNTER_LOG_ENABLE (1)

#if COUNTER_LOG_ENABLE
static void counter_logic_log_action(const char *event_name, int32_t delta, int32_t value)
{
    printf("[COUNTER] %s delta=%ld value=%ld\r\n", event_name, (long)delta, (long)value);
}

static void counter_logic_log_reset(int32_t value)
{
    printf("[COUNTER] RESET value=%ld\r\n", (long)value);
}
#else
static void counter_logic_log_action(const char *event_name, int32_t delta, int32_t value)
{
    LV_UNUSED(event_name);
    LV_UNUSED(delta);
    LV_UNUSED(value);
}

static void counter_logic_log_reset(int32_t value)
{
    LV_UNUSED(value);
}
#endif

static void counter_logic_apply_delta(counter_state_t *state, int32_t delta)
{
    /* Update model first, then clamp to keep beginner-friendly behavior. */
    state->value += delta;

    if(!state->allow_negative && state->value < 0) {
        state->value = 0;
    }

    /* Refresh the value shown on HMI immediately after model change. */
    counter_logic_refresh_label(state);
}

void counter_logic_refresh_label(counter_state_t *state)
{
    lv_label_set_text_fmt(state->value_label, "%ld", (long)state->value);
}

void counter_logic_init(counter_state_t *state, lv_obj_t *value_label, bool allow_negative)
{
    state->value = 0;
    state->value_label = value_label;
    state->allow_negative = allow_negative;
    counter_logic_refresh_label(state);
    counter_logic_log_action("INIT", 0, state->value);
}

void counter_logic_button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    counter_button_action_t *action = (counter_button_action_t *)lv_event_get_user_data(e);

    if(action == NULL || action->state == NULL) {
        return;
    }

    if(code == LV_EVENT_PRESSED) {
        counter_logic_apply_delta(action->state, action->tap_delta);
        counter_logic_log_action("TAP", action->tap_delta, action->state->value);
        return;
    }

    if(code == LV_EVENT_LONG_PRESSED_REPEAT) {
        counter_logic_apply_delta(action->state, action->hold_repeat_delta);
        counter_logic_log_action("HOLD_REPEAT", action->hold_repeat_delta, action->state->value);
    }
}

void counter_logic_reset_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code != LV_EVENT_PRESSED && code != LV_EVENT_CLICKED) {
        return;
    }

    counter_state_t *state = (counter_state_t *)lv_event_get_user_data(e);

    if(state == NULL) {
        return;
    }

    state->value = 0;
    counter_logic_refresh_label(state);
    counter_logic_log_reset(state->value);
}
