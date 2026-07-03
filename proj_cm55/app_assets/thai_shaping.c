/*******************************************************************************
 * thai_shaping.c — Layer 1 implementation: Thai cluster -> PUA substitute.
 *
 * Algorithm: walk the input byte-by-byte; at every position try the longest
 * prefix match against the static cluster_table (sorted longest-first by the
 * generator so the first match is the longest). On match, write the PUA
 * codepoint as 3-byte UTF-8 and skip ahead. On no match, copy one byte.
 *
 * Sara-am note: U+0E33 is the NFC composed form whose NFD is
 * <U+0E4D, U+0E32>. The current cluster table does not yet contain sara-am-
 * with-tone-mark clusters, so no normalisation is needed today — if/when we
 * add such clusters, expand U+0E33 -> NFD before the longest-match scan.
 *
 * No malloc. No global state beyond the static table. Reentrant. Depends
 * only on libc (memcmp, strlen).
 ******************************************************************************/
#include "thai_shaping.h"
#include "cluster_table.h"

#include <string.h>

/* Encode a Unicode codepoint in U+0080..U+FFFF range as 3-byte UTF-8.
 * Returns bytes written, or 0 if `out_sz < 3`. We never need 4-byte
 * encodings here — PUA U+E001..U+E0FF fits in 3 bytes. */
static inline size_t encode_pua_utf8(uint32_t cp, char *out, size_t out_sz)
{
    if (out_sz < 3) return 0;
    out[0] = (char)(0xE0 | ((cp >> 12) & 0x0F));
    out[1] = (char)(0x80 | ((cp >>  6) & 0x3F));
    out[2] = (char)(0x80 | ( cp        & 0x3F));
    return 3;
}

size_t thai_to_pua(const char *in, char *out, size_t out_sz)
{
    if (in == NULL || out == NULL || out_sz == 0) return 0;

    size_t in_pos = 0;
    size_t out_pos = 0;
    const size_t in_len = strlen(in);

    while (in_pos < in_len) {
        /* TODO(sara-am): if (in[in_pos] == 0xE0 && in[in_pos+1] == 0xB8 &&
         *                   in[in_pos+2] == 0xB3) emit NFD <0E4D, 0E32>
         *                   and continue the longest-match from in_pos+3.
         * Skipped today — no sara-am-with-tone clusters in cluster_table. */

        /* Longest-first match against the static table. The table is sorted
         * longest-first in the generator, so the first hit IS the longest. */
        const thai_cluster_t *match = NULL;
        for (size_t k = 0; k < THAI_CLUSTER_TABLE_LEN; k++) {
            const thai_cluster_t *e = &thai_cluster_table[k];
            if (in_pos + e->byte_len > in_len) continue;
            if (memcmp(in + in_pos, e->utf8, e->byte_len) == 0) {
                match = e;
                break;
            }
        }

        if (match != NULL) {
            size_t w = encode_pua_utf8(match->pua, out + out_pos, out_sz - 1 - out_pos);
            if (w == 0) break;          /* out buffer full — truncate cleanly */
            out_pos += w;
            in_pos  += match->byte_len;
        } else {
            /* No cluster match — copy one input byte as-is. UTF-8 self-
             * synchronisation means this is safe at any byte offset. */
            if (out_pos + 1 >= out_sz) break;
            out[out_pos++] = in[in_pos++];
        }
    }

    out[out_pos] = '\0';
    return out_pos;
}
