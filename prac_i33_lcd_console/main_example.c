/**
 * @file    main_example.c
 * @brief   Rich Text LCD Console - terminal-style logging on LVGL display
 *
 * @description
 *   Terminal emulator on the AMOLED display with color-coded log levels,
 *   timestamps from xTaskGetTickCount(), auto-scroll, and a 50-line ring
 *   buffer. Provides a reusable console_print() API for embedding in any
 *   project as a visual debug output.
 *
 *   Log levels and colors:
 *     [I] INFO   - green   (0x4CAF50)
 *     [W] WARN   - orange  (0xFF9800)
 *     [E] ERROR  - red     (0xF44336)
 *     [D] DEBUG  - gray    (0x808080)
 *
 * @board   AI Kit, Eva Kit, Game Console (all boards)
 * @author  TESAIoT
 */

#include "pse84_common.h"
#include "app_interface.h"
#include "tesaiot_thai.h"

/* ── Configuration ─────────────────────────────────────────────────── */
#define CONSOLE_MAX_LINES    50
#define CONSOLE_LINE_LEN     80
#define DEMO_INTERVAL_MS     500

/* ── Colors ────────────────────────────────────────────────────────── */
#define COLOR_BG             lv_color_hex(0x0D1B2A)
#define COLOR_CARD           lv_color_hex(0x142240)
#define COLOR_TEXT           lv_color_hex(0xE0E0E0)
#define COLOR_INFO           lv_color_hex(0x4CAF50)
#define COLOR_WARN           lv_color_hex(0xFF9800)
#define COLOR_ERROR          lv_color_hex(0xF44336)
#define COLOR_DEBUG          lv_color_hex(0x808080)

/* ── Log level enum ────────────────────────────────────────────────── */
typedef enum {
    LOG_INFO = 0,
    LOG_WARN,
    LOG_ERROR,
    LOG_DEBUG,
} log_level_t;

static const char *s_level_prefix[] = { "[I]", "[W]", "[E]", "[D]" };

/* ── Ring buffer ───────────────────────────────────────────────────── */
static char       s_lines[CONSOLE_MAX_LINES][CONSOLE_LINE_LEN];
static log_level_t s_levels[CONSOLE_MAX_LINES];
static uint16_t   s_head;        /* next write index           */
static uint16_t   s_count;       /* lines currently stored     */

/* ── UI handles ────────────────────────────────────────────────────── */
static lv_obj_t  *s_console_ta;
static lv_obj_t  *s_stats_label;
static uint32_t   s_total_lines;

/* ── Ring buffer helpers ───────────────────────────────────────────── */
static void ring_push(log_level_t level, const char *text)
{
    strncpy(s_lines[s_head], text, CONSOLE_LINE_LEN - 1);
    s_lines[s_head][CONSOLE_LINE_LEN - 1] = '\0';
    s_levels[s_head] = level;
    s_head = (s_head + 1) % CONSOLE_MAX_LINES;
    if (s_count < CONSOLE_MAX_LINES) s_count++;
    s_total_lines++;
}

/* ── Rebuild console display text ──────────────────────────────────── */
static void rebuild_console_text(void)
{
    /* Build a single string from ring buffer for the textarea.
     * Color is not per-character in lv_textarea, so we use level prefixes
     * to indicate severity visually. */
    static char buf[CONSOLE_MAX_LINES * CONSOLE_LINE_LEN];
    buf[0] = '\0';

    uint16_t start;
    if (s_count < CONSOLE_MAX_LINES) {
        start = 0;
    } else {
        start = s_head;  /* oldest entry */
    }

    for (uint16_t i = 0; i < s_count; i++) {
        uint16_t idx = (start + i) % CONSOLE_MAX_LINES;
        size_t cur_len = strlen(buf);
        size_t remaining = sizeof(buf) - cur_len - 1;
        if (remaining < CONSOLE_LINE_LEN + 2) break;

        if (i > 0) {
            buf[cur_len] = '\n';
            cur_len++;
            buf[cur_len] = '\0';
        }
        snprintf(buf + cur_len, remaining, "%s", s_lines[idx]);
    }

    lv_textarea_set_text(s_console_ta, buf);

    /* Auto-scroll to bottom */
    lv_textarea_set_cursor_pos(s_console_ta, LV_TEXTAREA_CURSOR_LAST);
}

/* ── Public API: console_print ─────────────────────────────────────── */
static void console_print(log_level_t level, const char *fmt, ...)
{
    char line[CONSOLE_LINE_LEN];
    uint32_t ticks = xTaskGetTickCount();
    uint32_t sec = ticks / configTICK_RATE_HZ;
    uint32_t ms  = (ticks % configTICK_RATE_HZ) * 1000 / configTICK_RATE_HZ;

    /* Format: [L] MM:SS.mmm message */
    int prefix_len = snprintf(line, sizeof(line), "%s %02u:%02u.%03u ",
                               s_level_prefix[level],
                               (unsigned)(sec / 60) % 100,
                               (unsigned)(sec % 60),
                               (unsigned)ms);

    va_list args;
    va_start(args, fmt);
    vsnprintf(line + prefix_len, sizeof(line) - prefix_len, fmt, args);
    va_end(args);

    ring_push(level, line);
    rebuild_console_text();

    /* Update stats */
    lv_label_set_text_fmt(s_stats_label, "Lines: %u / %u  |  Total: %u",
                          (unsigned)s_count, CONSOLE_MAX_LINES,
                          (unsigned)s_total_lines);
}

/* ── Demo timer: generate sample log messages ──────────────────────── */
static uint32_t s_demo_tick;

static void demo_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    s_demo_tick++;

    /* Cycle through different message types */
    switch (s_demo_tick % 8) {
    case 0:
        console_print(LOG_INFO, "BMI270 accel: x=%.2f y=%.2f z=%.2f g",
                      0.01f * (s_demo_tick % 100),
                      -0.5f + 0.02f * (s_demo_tick % 50),
                      9.78f + 0.01f * (s_demo_tick % 20));
        break;
    case 1:
        console_print(LOG_INFO, "DPS368 pressure: %.1f hPa, temp: %.1f C",
                      1013.25f + 0.1f * (s_demo_tick % 30),
                      25.0f + 0.1f * (s_demo_tick % 10));
        break;
    case 2:
        console_print(LOG_DEBUG, "IPC msg rx: cmd=0x%02X len=%u",
                      (unsigned)(0xC0 + s_demo_tick % 16),
                      (unsigned)(4 + s_demo_tick % 12));
        break;
    case 3:
        console_print(LOG_INFO, "WiFi RSSI: -%u dBm, TX credits: %u",
                      (unsigned)(40 + s_demo_tick % 30),
                      (unsigned)(1 + s_demo_tick % 5));
        break;
    case 4:
        console_print(LOG_WARN, "Heap free: %u bytes (%.0f%% used)",
                      (unsigned)(32768 - s_demo_tick * 16 % 16384),
                      50.0f + 0.5f * (s_demo_tick % 40));
        break;
    case 5:
        console_print(LOG_DEBUG, "LVGL refr: %u ms, fps: %u",
                      (unsigned)(8 + s_demo_tick % 12),
                      (unsigned)(60 - s_demo_tick % 20));
        break;
    case 6:
        if (s_demo_tick % 24 == 6) {
            console_print(LOG_ERROR, "Sensor timeout: BMM350 I2C NACK (0x15)");
        } else {
            console_print(LOG_INFO, "BMM350 compass: heading=%.1f deg",
                          360.0f * (s_demo_tick % 360) / 360.0f);
        }
        break;
    case 7:
        console_print(LOG_INFO, "System uptime: %u sec, tasks: 6",
                      (unsigned)(s_demo_tick * DEMO_INTERVAL_MS / 1000));
        break;
    }
}

/* ── Entry point ───────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    tesaiot_add_thai_support_badge();

    /* Reset state */
    s_head        = 0;
    s_count       = 0;
    s_total_lines = 0;
    s_demo_tick   = 0;
    memset(s_lines, 0, sizeof(s_lines));

    lv_obj_set_style_bg_color(parent, COLOR_BG, 0);

    /* ── Title bar ─────────────────────────────────────────────────── */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_LIST " LCD Console");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "คอนโซล LCD");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 28);

    /* ── Level legend ──────────────────────────────────────────────── */
    lv_obj_t *legend = lv_label_create(parent);
    lv_label_set_text(legend, "[I] Info   [W] Warn   [E] Error   [D] Debug");
    lv_obj_set_style_text_font(legend, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(legend, lv_color_hex(0x808080), 0);
    lv_obj_align(legend, LV_ALIGN_TOP_MID, 0, 32);

    /* ── Console textarea ──────────────────────────────────────────── */
    lv_obj_t *console_card = example_card_create(parent, 460, 320, COLOR_CARD);
    lv_obj_align(console_card, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_pad_all(console_card, 4, 0);

    s_console_ta = lv_textarea_create(console_card);
    lv_obj_set_size(s_console_ta, 448, 308);
    lv_obj_center(s_console_ta);
    lv_textarea_set_text(s_console_ta, "");
    lv_obj_set_style_bg_color(s_console_ta, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_text_color(s_console_ta, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(s_console_ta, &lv_font_montserrat_14, 0);
    lv_obj_set_style_border_width(s_console_ta, 0, 0);
    lv_obj_set_style_radius(s_console_ta, 4, 0);
    lv_obj_remove_flag(s_console_ta, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    /* ── Stats bar ─────────────────────────────────────────────────── */
    s_stats_label = lv_label_create(parent);
    lv_label_set_text_fmt(s_stats_label, "Lines: 0 / %u  |  Total: 0",
                          CONSOLE_MAX_LINES);
    lv_obj_set_style_text_font(s_stats_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_stats_label, UI_COLOR_PRIMARY, 0);
    lv_obj_align(s_stats_label, LV_ALIGN_TOP_MID, 0, 382);

    /* ── Initial boot messages ─────────────────────────────────────── */
    console_print(LOG_INFO,  "Console initialized");
    console_print(LOG_INFO,  "Display: %dx%d AMOLED", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    console_print(LOG_INFO,  "LVGL 9.2, FreeRTOS %s", tskKERNEL_VERSION_NUMBER);
    console_print(LOG_DEBUG, "Ring buffer: %d lines x %d chars",
                  CONSOLE_MAX_LINES, CONSOLE_LINE_LEN);
    console_print(LOG_INFO,  "Starting demo output every %d ms...",
                  DEMO_INTERVAL_MS);

    /* ── Start demo timer ──────────────────────────────────────────── */
    lv_timer_create(demo_timer_cb, DEMO_INTERVAL_MS, NULL);
}
