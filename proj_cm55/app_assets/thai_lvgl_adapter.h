/*******************************************************************************
 * thai_lvgl_adapter.h — Layer 2: LVGL convenience wrapper around L1 shaping.
 *
 * Pulls in L1 (thai_shaping.h) and adds an lv_label_set_text() variant that
 * runs PUA substitution first. Use this in any LVGL project; if you are NOT
 * using LVGL, ignore this header and call thai_to_pua() from L1 directly.
 *
 * Why a separate header? L1 is renderer-agnostic — a Qt or SDL2 user must
 * not be forced to drag in LVGL types. L2 isolates the dependency.
 ******************************************************************************/
#ifndef THAI_LVGL_ADAPTER_H
#define THAI_LVGL_ADAPTER_H

#include "thai_shaping.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward-declare LVGL's opaque label type so callers can use this header
 * without #including lvgl.h first. */
struct _lv_obj_t;

/* Per-call PUA scratch buffer size. Strings longer than this are still
 * substituted but only the first THAI_LABEL_SCRATCH_BYTES-1 bytes of OUTPUT
 * fit; the function silently truncates at a clean UTF-8 boundary. Bump if
 * your UI shows multi-paragraph Thai. */
#define THAI_LABEL_SCRATCH_BYTES 384

/* Convenience: thai_to_pua() the text into a static scratch buffer, then
 * call lv_label_set_text(). Single-threaded GFX-task assumption (LVGL is
 * not thread-safe anyway, so this is no extra constraint). */
void thai_label_set_text(struct _lv_obj_t *label, const char *text);

#ifdef __cplusplus
}
#endif

#endif /* THAI_LVGL_ADAPTER_H */
