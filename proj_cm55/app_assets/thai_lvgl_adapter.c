/*******************************************************************************
 * thai_lvgl_adapter.c — Layer 2 implementation: LVGL convenience wrapper.
 *
 * Only file in the project that talks to LVGL directly for Thai shaping.
 * Compile this file ONLY if your project links against LVGL. If you are
 * embedding the L1 core into a non-LVGL renderer, simply omit this TU and
 * call thai_to_pua() yourself, then feed the result to your renderer.
 ******************************************************************************/
#include "thai_lvgl_adapter.h"

#include <string.h>

/* Local prototype keeps this TU compiling without forcing lvgl.h. The real
 * declaration in lvgl.h is identical (opaque obj pointer + const char*). */
extern void lv_label_set_text(struct _lv_obj_t *label, const char *text);

/* Single-task scratch (GFX/LVGL task only). Static reuse — no malloc, no
 * stack pressure across deep render call chains. */
static char s_thai_scratch[THAI_LABEL_SCRATCH_BYTES];

void thai_label_set_text(struct _lv_obj_t *label, const char *text)
{
    if (text == NULL) {
        lv_label_set_text(label, "");
        return;
    }
    /* Fast path: pure ASCII (no Thai bytes 0xE0). thai_to_pua is also fast
     * on ASCII but this skips even the function-call cost — labels redraw
     * constantly in some UI loops. */
    int has_thai = 0;
    for (const char *p = text; *p; p++) {
        if ((unsigned char)*p == 0xE0) { has_thai = 1; break; }
    }
    if (!has_thai) {
        lv_label_set_text(label, text);
        return;
    }
    thai_to_pua(text, s_thai_scratch, sizeof(s_thai_scratch));
    lv_label_set_text(label, s_thai_scratch);
}
