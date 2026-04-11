#include "mic_view.h"

#include <stdbool.h>

#include "lvgl.h"

typedef struct
{
    lv_obj_t *left_ui_label;
    lv_obj_t *right_ui_label;
    lv_obj_t *left_stats_label;
    lv_obj_t *right_stats_label;

    lv_obj_t *left_ui_bar;
    lv_obj_t *right_ui_bar;
    lv_obj_t *left_avg_fs_bar;
    lv_obj_t *right_avg_fs_bar;
    lv_obj_t *left_peak_fs_bar;
    lv_obj_t *right_peak_fs_bar;

    lv_obj_t *left_avg_nf_line;
    lv_obj_t *right_avg_nf_line;

    uint32_t left_peak_hold_pct;
    uint32_t right_peak_hold_pct;
    uint32_t left_hold_tick_ms;
    uint32_t right_hold_tick_ms;

    lv_obj_t *balance_bar;
    lv_obj_t *balance_label;
    lv_obj_t *footer_status;
} mic_view_ctx_t;

static mic_view_ctx_t s_ctx = {0};
static bool s_view_created = false;

#define MIC_PEAK_HOLD_MS              (1500U)
#define MIC_PEAK_DECAY_INTERVAL_MS    (100U)
#define MIC_PEAK_DECAY_STEP_PCT       (2U)

#define MIC_UI_LOW_THR_PCT            (35U)
#define MIC_UI_HIGH_THR_PCT           (70U)
#define MIC_NOISE_FLOOR_PCT           (2U)

static void style_card(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x122034), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x274766), LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_row(obj, 4, LV_PART_MAIN);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static void style_metric_bar(lv_obj_t *bar, lv_color_t indicator_color)
{
    lv_obj_set_width(bar, LV_PCT(100));
    lv_obj_set_height(bar, 10);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_radius(bar, 6, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 6, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x1D3555), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, indicator_color, LV_PART_INDICATOR);
}

static uint32_t clamp_percent_from_tenth(uint32_t tenth_pct)
{
    uint32_t pct = tenth_pct / 10U;
    return (pct > 100U) ? 100U : pct;
}

static lv_color_t level_to_color(uint32_t ui_pct)
{
    if (ui_pct < MIC_UI_LOW_THR_PCT)
    {
        return lv_color_hex(0x22C55E);
    }

    if (ui_pct < MIC_UI_HIGH_THR_PCT)
    {
        return lv_color_hex(0xF59E0B);
    }

    return lv_color_hex(0xEF4444);
}

/* Draw a fixed noise-floor marker on Avg %FS bars. */
static void set_noise_floor_line_pos(lv_obj_t *bar, lv_obj_t *line, uint32_t floor_pct)
{
    if ((NULL == bar) || (NULL == line))
    {
        return;
    }

    int32_t w = lv_obj_get_width(bar);
    int32_t h = lv_obj_get_height(bar);
    if ((w <= 2) || (h <= 2))
    {
        return;
    }

    int32_t x = (int32_t)((w * (int32_t)floor_pct) / 100);
    if (x < 1)
    {
        x = 1;
    }
    if (x > (w - 2))
    {
        x = w - 2;
    }

    lv_obj_set_size(line, 2, h - 2);
    lv_obj_set_pos(line, x, 1);
}

/* Keep a short peak-hold value and then decay smoothly for readability. */
static void update_peak_hold(uint32_t current_peak_pct, uint32_t *hold_pct, uint32_t *hold_tick_ms)
{
    uint32_t now_ms = lv_tick_get();

    if ((current_peak_pct >= *hold_pct) || (0U == *hold_tick_ms))
    {
        *hold_pct = current_peak_pct;
        *hold_tick_ms = now_ms;
        return;
    }

    uint32_t elapsed_ms = lv_tick_elaps(*hold_tick_ms);
    if (elapsed_ms <= MIC_PEAK_HOLD_MS)
    {
        return;
    }

    uint32_t decay_elapsed = elapsed_ms - MIC_PEAK_HOLD_MS;
    uint32_t steps = decay_elapsed / MIC_PEAK_DECAY_INTERVAL_MS;
    if (steps == 0U)
    {
        return;
    }

    uint32_t decay = steps * MIC_PEAK_DECAY_STEP_PCT;
    if (*hold_pct > decay)
    {
        *hold_pct -= decay;
    }
    else
    {
        *hold_pct = 0U;
    }

    if (*hold_pct < current_peak_pct)
    {
        *hold_pct = current_peak_pct;
    }

    *hold_tick_ms = now_ms;
}

static lv_obj_t *create_channel_card(lv_obj_t *parent,
                                     const char *title,
                                     lv_obj_t **ui_label,
                                     lv_obj_t **ui_bar,
                                     lv_obj_t **avg_fs_bar,
                                     lv_obj_t **avg_nf_line,
                                     lv_obj_t **peak_fs_bar,
                                     lv_obj_t **stats_label)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_flex_grow(card, 1);
    lv_obj_set_height(card, LV_PCT(100));
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    style_card(card);

    lv_obj_t *title_label = lv_label_create(card);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0x67E8F9), LV_PART_MAIN);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, LV_PART_MAIN);

    *ui_label = lv_label_create(card);
    lv_label_set_text(*ui_label, "UI Level: 0%");
    lv_obj_set_style_text_color(*ui_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(*ui_label, &lv_font_montserrat_20, LV_PART_MAIN);

    *ui_bar = lv_bar_create(card);
    style_metric_bar(*ui_bar, lv_color_hex(0x334155));

    lv_obj_t *avg_caption = lv_label_create(card);
    lv_label_set_text(avg_caption, "Avg %FS");
    lv_obj_set_style_text_color(avg_caption, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(avg_caption, &lv_font_montserrat_14, LV_PART_MAIN);

    *avg_fs_bar = lv_bar_create(card);
    style_metric_bar(*avg_fs_bar, lv_color_hex(0x3B82F6));

    *avg_nf_line = lv_obj_create(*avg_fs_bar);
    lv_obj_set_style_bg_color(*avg_nf_line, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(*avg_nf_line, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_border_width(*avg_nf_line, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(*avg_nf_line, 0, LV_PART_MAIN);
    lv_obj_clear_flag(*avg_nf_line, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *peak_caption = lv_label_create(card);
    lv_label_set_text(peak_caption, "Peak %FS (Hold)");
    lv_obj_set_style_text_color(peak_caption, lv_color_hex(0xFDBA74), LV_PART_MAIN);
    lv_obj_set_style_text_font(peak_caption, &lv_font_montserrat_14, LV_PART_MAIN);

    *peak_fs_bar = lv_bar_create(card);
    style_metric_bar(*peak_fs_bar, lv_color_hex(0xF97316));

    /* Push summary stats to bottom for consistent visual balance. */
    lv_obj_t *spacer = lv_obj_create(card);
    lv_obj_set_size(spacer, LV_PCT(100), 1);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(spacer, 0, LV_PART_MAIN);
    lv_obj_clear_flag(spacer, LV_OBJ_FLAG_CLICKABLE);

    *stats_label = lv_label_create(card);
    lv_label_set_text(*stats_label, "PeakAbs 0 | AvgAbs 0");
    lv_obj_set_style_text_color(*stats_label, lv_color_hex(0xA5B4FC), LV_PART_MAIN);
    lv_obj_set_style_text_font(*stats_label, &lv_font_montserrat_14, LV_PART_MAIN);

    return card;
}

cy_rslt_t mic_view_create(void)
{
    if (s_view_created)
    {
        return CY_RSLT_SUCCESS;
    }

    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0B1220), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(screen, 2, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(screen, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);

    /* Header */
    lv_obj_t *header = lv_obj_create(screen);
    lv_obj_set_width(header, LV_PCT(100));
    lv_obj_set_height(header, LV_SIZE_CONTENT);
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(header, 4, LV_PART_MAIN);

    lv_obj_t *title_wrap = lv_obj_create(header);
    lv_obj_set_layout(title_wrap, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(title_wrap, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_opa(title_wrap, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(title_wrap, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(title_wrap, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(title_wrap, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(title_wrap, 4, LV_PART_MAIN);
    lv_obj_set_width(title_wrap, LV_PCT(100));
    lv_obj_set_height(title_wrap, LV_SIZE_CONTENT);

    lv_obj_t *title = lv_label_create(title_wrap);
    lv_label_set_text(title, "Digital Mic Stereo Monitor");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN);

    lv_obj_t *subtitle = lv_label_create(title_wrap);
    lv_label_set_text(subtitle, "Stereo level + balance monitor");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_14, LV_PART_MAIN);

    /* Content */
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_width(content, LV_PCT(100));
    lv_obj_set_height(content, LV_PCT(100));
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(content, 4, LV_PART_MAIN);

    lv_obj_t *top_row = lv_obj_create(content);
    lv_obj_set_width(top_row, LV_PCT(100));
    lv_obj_set_flex_grow(top_row, 1);
    lv_obj_set_layout(top_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(top_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_opa(top_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(top_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(top_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(top_row, 6, LV_PART_MAIN);
    lv_obj_clear_flag(top_row, LV_OBJ_FLAG_SCROLLABLE);

    (void)create_channel_card(top_row,
                              "Left Channel",
                              &s_ctx.left_ui_label,
                              &s_ctx.left_ui_bar,
                              &s_ctx.left_avg_fs_bar,
                              &s_ctx.left_avg_nf_line,
                              &s_ctx.left_peak_fs_bar,
                              &s_ctx.left_stats_label);

    (void)create_channel_card(top_row,
                              "Right Channel",
                              &s_ctx.right_ui_label,
                              &s_ctx.right_ui_bar,
                              &s_ctx.right_avg_fs_bar,
                              &s_ctx.right_avg_nf_line,
                              &s_ctx.right_peak_fs_bar,
                              &s_ctx.right_stats_label);

    lv_obj_t *balance_card = lv_obj_create(content);
    lv_obj_set_width(balance_card, LV_PCT(100));
    lv_obj_set_height(balance_card, 72);
    lv_obj_set_layout(balance_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(balance_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(balance_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    style_card(balance_card);

    lv_obj_t *balance_title = lv_label_create(balance_card);
    lv_label_set_text(balance_title, "Balance (L-R)");
    lv_obj_set_style_text_color(balance_title, lv_color_hex(0xBFDBFE), LV_PART_MAIN);
    lv_obj_set_style_text_font(balance_title, &lv_font_montserrat_14, LV_PART_MAIN);

    s_ctx.balance_bar = lv_bar_create(balance_card);
    lv_obj_set_width(s_ctx.balance_bar, LV_PCT(100));
    lv_obj_set_height(s_ctx.balance_bar, 8);
    lv_bar_set_range(s_ctx.balance_bar, -100, 100);
    lv_bar_set_value(s_ctx.balance_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_radius(s_ctx.balance_bar, 7, LV_PART_MAIN);
    lv_obj_set_style_radius(s_ctx.balance_bar, 7, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(s_ctx.balance_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_ctx.balance_bar, lv_color_hex(0x1D3555), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_ctx.balance_bar, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);

    s_ctx.balance_label = lv_label_create(balance_card);
    lv_label_set_text(s_ctx.balance_label, "Balance: 0 (CENTER)");
    lv_obj_set_style_text_color(s_ctx.balance_label, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.balance_label, &lv_font_montserrat_14, LV_PART_MAIN);

    /* Footer */
    lv_obj_t *footer = lv_obj_create(screen);
    lv_obj_set_width(footer, LV_PCT(100));
    lv_obj_set_height(footer, LV_SIZE_CONTENT);
    lv_obj_set_layout(footer, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(footer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(footer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(footer, 8, LV_PART_MAIN);

    s_ctx.footer_status = lv_label_create(footer);
    lv_label_set_text(s_ctx.footer_status, "Frame: 0 | Lavg: 0 | Ravg: 0");
    lv_obj_set_style_text_color(s_ctx.footer_status, lv_color_hex(0xFDE68A), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.footer_status, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_flex_grow(s_ctx.footer_status, 1);

    lv_obj_t *footer_hint = lv_label_create(footer);
    lv_label_set_text(footer_hint, "Live PDM | Read-only");
    lv_obj_set_style_text_color(footer_hint, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(footer_hint, &lv_font_montserrat_14, LV_PART_MAIN);

    s_ctx.left_hold_tick_ms = lv_tick_get();
    s_ctx.right_hold_tick_ms = s_ctx.left_hold_tick_ms;

    set_noise_floor_line_pos(s_ctx.left_avg_fs_bar, s_ctx.left_avg_nf_line, MIC_NOISE_FLOOR_PCT);
    set_noise_floor_line_pos(s_ctx.right_avg_fs_bar, s_ctx.right_avg_nf_line, MIC_NOISE_FLOOR_PCT);

    s_view_created = true;
    return CY_RSLT_SUCCESS;
}

void mic_view_apply(const mic_presenter_sample_t *sample)
{
    if ((!s_view_created) || (NULL == sample))
    {
        return;
    }

    uint32_t left_avg_fs = clamp_percent_from_tenth(sample->left_avg_tenth_pct_fs);
    uint32_t right_avg_fs = clamp_percent_from_tenth(sample->right_avg_tenth_pct_fs);
    uint32_t left_peak_fs_curr = clamp_percent_from_tenth(sample->left_peak_tenth_pct_fs);
    uint32_t right_peak_fs_curr = clamp_percent_from_tenth(sample->right_peak_tenth_pct_fs);

    update_peak_hold(left_peak_fs_curr, &s_ctx.left_peak_hold_pct, &s_ctx.left_hold_tick_ms);
    update_peak_hold(right_peak_fs_curr, &s_ctx.right_peak_hold_pct, &s_ctx.right_hold_tick_ms);

    lv_color_t left_color = level_to_color(sample->left_ui_pct);
    lv_color_t right_color = level_to_color(sample->right_ui_pct);

    lv_label_set_text_fmt(s_ctx.left_ui_label, "UI Level: %lu%%", (unsigned long)sample->left_ui_pct);
    lv_label_set_text_fmt(s_ctx.right_ui_label, "UI Level: %lu%%", (unsigned long)sample->right_ui_pct);

    lv_bar_set_value(s_ctx.left_ui_bar, (int32_t)sample->left_ui_pct, LV_ANIM_ON);
    lv_bar_set_value(s_ctx.right_ui_bar, (int32_t)sample->right_ui_pct, LV_ANIM_ON);
    lv_obj_set_style_bg_color(s_ctx.left_ui_bar, left_color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_ctx.right_ui_bar, right_color, LV_PART_INDICATOR);

    lv_bar_set_value(s_ctx.left_avg_fs_bar, (int32_t)left_avg_fs, LV_ANIM_ON);
    lv_bar_set_value(s_ctx.right_avg_fs_bar, (int32_t)right_avg_fs, LV_ANIM_ON);

    lv_bar_set_value(s_ctx.left_peak_fs_bar, (int32_t)s_ctx.left_peak_hold_pct, LV_ANIM_ON);
    lv_bar_set_value(s_ctx.right_peak_fs_bar, (int32_t)s_ctx.right_peak_hold_pct, LV_ANIM_ON);

    lv_label_set_text_fmt(s_ctx.left_stats_label,
                          "Peak %lu%% (hold %lu%%) | AvgAbs %lu",
                          (unsigned long)left_peak_fs_curr,
                          (unsigned long)s_ctx.left_peak_hold_pct,
                          (unsigned long)sample->left_avg_abs);

    lv_label_set_text_fmt(s_ctx.right_stats_label,
                          "Peak %lu%% (hold %lu%%) | AvgAbs %lu",
                          (unsigned long)right_peak_fs_curr,
                          (unsigned long)s_ctx.right_peak_hold_pct,
                          (unsigned long)sample->right_avg_abs);

    set_noise_floor_line_pos(s_ctx.left_avg_fs_bar, s_ctx.left_avg_nf_line, MIC_NOISE_FLOOR_PCT);
    set_noise_floor_line_pos(s_ctx.right_avg_fs_bar, s_ctx.right_avg_nf_line, MIC_NOISE_FLOOR_PCT);

    lv_bar_set_value(s_ctx.balance_bar, sample->balance_lr, LV_ANIM_ON);
    lv_label_set_text_fmt(s_ctx.balance_label,
                          "Balance: %ld (%s)",
                          (long)sample->balance_lr,
                          (sample->balance_lr > 3) ? "L" : ((sample->balance_lr < -3) ? "R" : "CENTER"));

    lv_label_set_text_fmt(s_ctx.footer_status,
                          "Frame: %lu | Lavg: %lu | Ravg: %lu",
                          (unsigned long)sample->frame_count,
                          (unsigned long)sample->left_avg_abs,
                          (unsigned long)sample->right_avg_abs);
}