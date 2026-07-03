/*******************************************************************************
 * thai_shaping.h — Layer 1: Thai cluster -> PUA substitute (renderer-agnostic).
 *
 * LVGL (and any bitmap text renderer without a complex-text shaper) has no
 * GSUB/GPOS, so a Thai cluster like "ที่" (ท + ี + ่) renders as three
 * overlapping glyphs. This module substitutes known Thai cluster byte
 * sequences in a UTF-8 string with Private Use Area codepoints
 * (U+E001..U+E0FF) whose glyphs are pre-composed bitmaps produced by
 * HarfBuzz off-line (see tools/gen_pua_font.py).
 *
 * This header is the L1 PUBLIC API — pure UTF-8 in / UTF-8 out, no renderer
 * dependency. The only system dependency is libc (memcmp, strlen). Suitable
 * for any text-rendering engine (LVGL, SDL_ttf, Qt, custom). For an LVGL-
 * specific convenience wrapper, see thai_lvgl_adapter.h (L2).
 *
 * Boundary rule: PUA bytes MUST NOT leak out of the render path. Anything
 * that goes to a log, IPC, BLE notification, MQTT, or REPL must stay UTF-8
 * Thai. Only the rendered text sees PUA. The caller's source buffer is
 * never modified — substitute always writes to a fresh `out` buffer.
 ******************************************************************************/
#ifndef THAI_SHAPING_H
#define THAI_SHAPING_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Translate every known Thai cluster in `in` (UTF-8) to its PUA codepoint
 * (encoded as 3-byte UTF-8 for U+E001..U+E0FF). Unknown sequences pass
 * through unchanged. Returns the number of bytes written to `out`,
 * EXCLUDING the trailing NUL. Always NUL-terminates `out` if `out_sz >= 1`.
 *
 * Safe to call on non-Thai strings; ASCII passes through with zero cost.
 * Reentrant — no global state beyond the static cluster table. */
size_t thai_to_pua(const char *in, char *out, size_t out_sz);

#ifdef __cplusplus
}
#endif

#endif /* THAI_SHAPING_H */
