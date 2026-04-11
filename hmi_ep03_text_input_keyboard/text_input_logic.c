#include "text_input_logic.h"
#include <stdio.h>

#define TEXT_INPUT_LOG_ENABLE (1)

enum {
    TEXT_INPUT_MODE_NORMAL = 0,
    TEXT_INPUT_MODE_NUMBER = 1
};

static void text_input_log(const char *event_name, const char *source, const char *text)
{
#if TEXT_INPUT_LOG_ENABLE
    printf("[TEXT_INPUT] %s src=%s text='%s'\r\n", event_name, source, text);
#else
    LV_UNUSED(event_name);
    LV_UNUSED(source);
    LV_UNUSED(text);
#endif
}

static const char *text_input_source_name(const text_input_state_t *state, lv_obj_t *target)
{
    if(target == state->realtime_textarea) {
        return "realtime";
    }

    if(target == state->confirmed_textarea) {
        return "confirmed";
    }

    return "unknown";
}

static void text_input_show_keyboard(text_input_state_t *state)
{
    if(lv_obj_has_flag(state->keyboard, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_clear_flag(state->keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}

static void text_input_hide_keyboard(text_input_state_t *state)
{
    lv_obj_add_flag(state->keyboard, LV_OBJ_FLAG_HIDDEN);

    if(state->active_textarea != NULL) {
        lv_obj_clear_state(state->active_textarea, LV_STATE_FOCUSED);
    }
}

static void text_input_update_realtime_label(text_input_state_t *state)
{
    const char *text = lv_textarea_get_text(state->realtime_textarea);
    lv_label_set_text_fmt(state->realtime_label, "Realtime Label: %s", (text[0] != '\0') ? text : "-");
}

static void text_input_update_confirmed_label(text_input_state_t *state)
{
    const char *text = lv_textarea_get_text(state->confirmed_textarea);
    lv_label_set_text_fmt(state->confirmed_label, "Confirmed Label: %s", (text[0] != '\0') ? text : "-");
}

/* Apply keyboard mode and input filter for one target textarea. */
static void text_input_apply_mode_for_target(text_input_state_t *state, lv_obj_t *target)
{
    uint32_t selected_mode = lv_dropdown_get_selected(state->mode_dropdown);

    if(selected_mode == TEXT_INPUT_MODE_NUMBER) {
        lv_keyboard_set_mode(state->keyboard, LV_KEYBOARD_MODE_NUMBER);
        lv_textarea_set_accepted_chars(target, "0123456789");
        lv_textarea_set_placeholder_text(target, "Input number only");
        text_input_log("MODE_NUMBER", text_input_source_name(state, target), lv_textarea_get_text(target));
        return;
    }

    lv_keyboard_set_mode(state->keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_textarea_set_accepted_chars(target, NULL);
    lv_textarea_set_placeholder_text(target, "Type text here");
    text_input_log("MODE_NORMAL", text_input_source_name(state, target), lv_textarea_get_text(target));
}

/* Keep both textareas aligned to currently selected dropdown mode. */
static void text_input_apply_mode_for_all(text_input_state_t *state)
{
    text_input_apply_mode_for_target(state, state->realtime_textarea);
    text_input_apply_mode_for_target(state, state->confirmed_textarea);
}

void text_input_logic_init(text_input_state_t *state,
                           lv_obj_t *realtime_textarea,
                           lv_obj_t *confirmed_textarea,
                           lv_obj_t *realtime_label,
                           lv_obj_t *confirmed_label,
                           lv_obj_t *keyboard,
                           lv_obj_t *mode_dropdown)
{
    state->realtime_textarea = realtime_textarea;
    state->confirmed_textarea = confirmed_textarea;
    state->realtime_label = realtime_label;
    state->confirmed_label = confirmed_label;
    state->keyboard = keyboard;
    state->mode_dropdown = mode_dropdown;
    state->active_textarea = NULL;

    lv_label_set_text(state->realtime_label, "Realtime Label: -");
    lv_label_set_text(state->confirmed_label, "Confirmed Label: -");

    text_input_apply_mode_for_all(state);
    text_input_hide_keyboard(state);
    text_input_log("INIT", "system", "ready");
}

void text_input_logic_textarea_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    text_input_state_t *state = (text_input_state_t *)lv_event_get_user_data(e);
    lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);

    if(state == NULL || target == NULL) {
        return;
    }

    /* Keyboard opens only from input widgets (not from dropdown). */
    if(code == LV_EVENT_CLICKED || code == LV_EVENT_FOCUSED) {
        state->active_textarea = target;
        lv_keyboard_set_textarea(state->keyboard, target);
        text_input_apply_mode_for_target(state, target);
        text_input_show_keyboard(state);
        text_input_log("KB_SHOW", text_input_source_name(state, target), lv_textarea_get_text(target));
        return;
    }

    /* Realtime output is bound only to Input A. */
    if(code == LV_EVENT_VALUE_CHANGED && target == state->realtime_textarea) {
        text_input_update_realtime_label(state);
        text_input_log("REALTIME", "realtime", lv_textarea_get_text(target));
    }
}

void text_input_logic_keyboard_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    text_input_state_t *state = (text_input_state_t *)lv_event_get_user_data(e);

    if(state == NULL) {
        return;
    }

    if(code == LV_EVENT_READY) {
        /* Confirm behavior depends on which input currently owns keyboard focus. */
        if(state->active_textarea == state->confirmed_textarea) {
            text_input_update_confirmed_label(state);
            text_input_log("CONFIRM", "confirmed", lv_textarea_get_text(state->confirmed_textarea));
        } else if(state->active_textarea == state->realtime_textarea) {
            text_input_update_realtime_label(state);
            text_input_log("CONFIRM", "realtime", lv_textarea_get_text(state->realtime_textarea));
        }

        text_input_hide_keyboard(state);
        text_input_log("KB_HIDE", "system", "ready");
        return;
    }

    if(code == LV_EVENT_CANCEL) {
        if(state->active_textarea != NULL) {
            text_input_log("CANCEL", text_input_source_name(state, state->active_textarea),
                           lv_textarea_get_text(state->active_textarea));
        }

        text_input_hide_keyboard(state);
        text_input_log("KB_HIDE", "system", "cancel");
    }
}

void text_input_logic_mode_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    text_input_state_t *state = (text_input_state_t *)lv_event_get_user_data(e);

    if(state == NULL || code != LV_EVENT_VALUE_CHANGED) {
        return;
    }

    /* Dropdown updates mode only; it does not open keyboard. */
    text_input_apply_mode_for_all(state);

    if(state->active_textarea != NULL && !lv_obj_has_flag(state->keyboard, LV_OBJ_FLAG_HIDDEN)) {
        text_input_apply_mode_for_target(state, state->active_textarea);
    }
}
