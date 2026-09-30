# B18 — Thai Text Display

Demonstrates Thai script rendering on the PSoC Edge E84 display using
**Noto Sans Thai** fonts (4 sizes: 14, 16, 20, 28 px).

## Thai Rendering Complexity

Thai script is one of the most challenging Brahmic scripts to render correctly:

| Feature | Example | Challenge |
|---------|---------|-----------|
| Stacked vowels | สระ + วรรณยุกต์ | Up to 3 glyphs above/below base consonant |
| Sara Am (อำ) | ความสำคัญ | Composite: above vowel + final ม in single codepoint |
| Leading vowels | เ แ โ ใ ไ | Written before consonant, pronounced after |
| Consonant clusters | ทร, ปร, กล | Position-dependent glyph adjustment |
| Thai-ASCII mix | PSoC Edge E84 | Baseline alignment between scripts |

## Fonts Available

```c
#include "lv_fonts_thai.h"

&lv_font_noto_thai_14   /* Captions, metadata */
&lv_font_noto_thai_16   /* Body text */
&lv_font_noto_thai_20   /* Headings */
&lv_font_noto_thai_28   /* Titles, large display */
```

Coverage: ASCII (0x0020-0x007E) + Thai (0x0E00-0x0E7F), 4bpp anti-aliased.

## Board Compatibility

Works on all boards — no sensor dependency.
