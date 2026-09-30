/**
 * @file    main_example.c
 * @brief   Image Format Explorer - JPEG/PNG/BMP format details & compression concepts
 *
 * @description
 *   Educational image format reference UI showing JPEG, PNG, and BMP format
 *   internals: file structure, compression methods, color space details,
 *   and interactive compression ratio calculator. Displays format comparison
 *   table and animated color space visualization using LVGL canvas.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "pse84_common.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define NUM_FORMATS     3
#define CANVAS_SIZE     120
#define UPDATE_MS       100

/* Format tab indices */
#define FMT_JPEG        0
#define FMT_PNG         1
#define FMT_BMP         2

/* Color space visualization palette */
#define CS_RED          lv_color_hex(0xFF4444)
#define CS_GREEN        lv_color_hex(0x44FF44)
#define CS_BLUE         lv_color_hex(0x4444FF)
#define CS_YELLOW       lv_color_hex(0xFFFF44)
#define CS_CYAN         lv_color_hex(0x44FFFF)
#define CS_MAGENTA      lv_color_hex(0xFF44FF)

/* ---------------------------------------------------------------------------
 * Format data
 * --------------------------------------------------------------------------- */
typedef struct {
    const char *name;
    const char *icon;
    const char *extension;
    const char *compression;
    const char *color_depth;
    const char *transparency;
    const char *animation;
    const char *typical_ratio;
    const char *use_case;
    const char *structure;
    lv_color_t  accent;
} format_info_t;

static const format_info_t s_formats[NUM_FORMATS] = {
    {
        .name        = "JPEG",
        .icon        = LV_SYMBOL_IMAGE,
        .extension   = ".jpg / .jpeg",
        .compression = "Lossy DCT",
        .color_depth = "24-bit (8 per channel)",
        .transparency= "No",
        .animation   = "No",
        .typical_ratio = "10:1 to 20:1",
        .use_case    = "Photos, gradients",
        .structure   = "SOI > APP0 > DQT > SOF >\n"
                       "DHT > SOS > Data > EOI",
        .accent      = { .blue = 0x36, .green = 0x9E, .red = 0xF4 },  /* Red-orange */
    },
    {
        .name        = "PNG",
        .icon        = LV_SYMBOL_IMAGE,
        .extension   = ".png",
        .compression = "Lossless DEFLATE",
        .color_depth = "1/2/4/8/16 bpc + Alpha",
        .transparency= "Yes (alpha channel)",
        .animation   = "APNG extension",
        .typical_ratio = "2:1 to 5:1",
        .use_case    = "UI assets, screenshots",
        .structure   = "Signature > IHDR >\n"
                       "PLTE > IDAT > IEND",
        .accent      = { .blue = 0x50, .green = 0xAF, .red = 0x4C },  /* Green */
    },
    {
        .name        = "BMP",
        .icon        = LV_SYMBOL_IMAGE,
        .extension   = ".bmp / .dib",
        .compression = "None (or RLE)",
        .color_depth = "1/4/8/16/24/32 bpp",
        .transparency= "32-bit only",
        .animation   = "No",
        .typical_ratio = "1:1 (uncompressed)",
        .use_case    = "Raw data, framebuffers",
        .structure   = "File Header (14B) >\n"
                       "Info Header (40B) >\n"
                       "Color Table > Pixel Data",
        .accent      = { .blue = 0xF3, .green = 0x96, .red = 0x21 },  /* Blue */
    },
};

/* ---------------------------------------------------------------------------
 * Context
 * --------------------------------------------------------------------------- */
typedef struct {
    lv_obj_t   *parent;
    lv_obj_t   *tab_btns[NUM_FORMATS];
    lv_obj_t   *tab_labels[NUM_FORMATS];
    lv_obj_t   *detail_labels[7];       /* compression, depth, alpha, anim, ratio, use, structure */
    lv_obj_t   *format_title;
    lv_obj_t   *ext_label;

    /* Compression calculator */
    lv_obj_t   *calc_width_ta;
    lv_obj_t   *calc_height_ta;
    lv_obj_t   *calc_result;

    /* Color space canvas */
    lv_obj_t   *canvas;
    uint8_t     canvas_buf[LV_CANVAS_BUF_SIZE(CANVAS_SIZE, CANVAS_SIZE,
                                               LV_COLOR_FORMAT_RGB565, 0)];
    lv_timer_t *anim_timer;
    uint32_t    anim_tick;
    int         active_format;
} imgfmt_ctx_t;

static imgfmt_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * Update detail panel for selected format
 * --------------------------------------------------------------------------- */
static void update_details(imgfmt_ctx_t *ctx)
{
    const format_info_t *f = &s_formats[ctx->active_format];

    char title_buf[32];
    snprintf(title_buf, sizeof(title_buf), "%s %s Format", f->icon, f->name);
    lv_label_set_text(ctx->format_title, title_buf);
    lv_obj_set_style_text_color(ctx->format_title, f->accent, 0);

    char ext_buf[32];
    snprintf(ext_buf, sizeof(ext_buf), "Extension: %s", f->extension);
    lv_label_set_text(ctx->ext_label, ext_buf);

    const char *labels[] = {
        "Compression", "Color Depth", "Transparency",
        "Animation", "Typical Ratio", "Best For", "File Structure"
    };
    const char *values[] = {
        f->compression, f->color_depth, f->transparency,
        f->animation, f->typical_ratio, f->use_case, f->structure
    };

    for (int i = 0; i < 7; i++) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%s:\n  %s", labels[i], values[i]);
        lv_label_set_text(ctx->detail_labels[i], buf);
    }
}

/* ---------------------------------------------------------------------------
 * Tab button callback
 * --------------------------------------------------------------------------- */
static void tab_btn_cb(lv_event_t *e)
{
    imgfmt_ctx_t *ctx = (imgfmt_ctx_t *)lv_event_get_user_data(e);
    lv_obj_t *btn = lv_event_get_target(e);

    for (int i = 0; i < NUM_FORMATS; i++) {
        if (ctx->tab_btns[i] == btn) {
            ctx->active_format = i;
            lv_obj_set_style_bg_color(btn, s_formats[i].accent, 0);
            lv_obj_set_style_text_color(ctx->tab_labels[i],
                                         lv_color_hex(0xFFFFFF), 0);
        } else {
            lv_obj_set_style_bg_color(ctx->tab_btns[i], UI_COLOR_CARD_BG, 0);
            lv_obj_set_style_text_color(ctx->tab_labels[i], UI_COLOR_TEXT_DIM, 0);
        }
    }

    update_details(ctx);
}

/* ---------------------------------------------------------------------------
 * Compression calculator callback
 * --------------------------------------------------------------------------- */
static void calc_cb(lv_event_t *e)
{
    imgfmt_ctx_t *ctx = (imgfmt_ctx_t *)lv_event_get_user_data(e);

    const char *w_str = lv_textarea_get_text(ctx->calc_width_ta);
    const char *h_str = lv_textarea_get_text(ctx->calc_height_ta);

    int w = atoi(w_str);
    int h = atoi(h_str);

    if (w <= 0 || h <= 0 || w > 8192 || h > 8192) {
        lv_label_set_text(ctx->calc_result, "Invalid dimensions");
        lv_obj_set_style_text_color(ctx->calc_result, UI_COLOR_ERROR, 0);
        return;
    }

    uint32_t raw_rgb565 = (uint32_t)w * (uint32_t)h * 2;
    uint32_t raw_rgb888 = (uint32_t)w * (uint32_t)h * 3;

    /* Estimated sizes per format */
    uint32_t jpeg_est = raw_rgb888 / 15;  /* ~15:1 typical */
    uint32_t png_est  = raw_rgb888 / 3;   /* ~3:1 typical  */
    uint32_t bmp_size = raw_rgb888 + 54;  /* header + raw  */

    char buf[256];
    snprintf(buf, sizeof(buf),
             "Raw RGB565: %lu KB\n"
             "Raw RGB888: %lu KB\n"
             "JPEG ~15:1: %lu KB\n"
             "PNG  ~3:1:  %lu KB\n"
             "BMP  1:1:   %lu KB",
             (unsigned long)(raw_rgb565 / 1024),
             (unsigned long)(raw_rgb888 / 1024),
             (unsigned long)(jpeg_est / 1024),
             (unsigned long)(png_est / 1024),
             (unsigned long)(bmp_size / 1024));

    lv_label_set_text(ctx->calc_result, buf);
    lv_obj_set_style_text_color(ctx->calc_result, UI_COLOR_TEXT, 0);
}

/* ---------------------------------------------------------------------------
 * Canvas animation timer - draw color space visualization
 * --------------------------------------------------------------------------- */
static void anim_timer_cb(lv_timer_t *timer)
{
    imgfmt_ctx_t *ctx = (imgfmt_ctx_t *)lv_timer_get_user_data(timer);
    ctx->anim_tick++;

    lv_canvas_fill_bg(ctx->canvas, lv_color_hex(0x0A0A14), LV_OPA_COVER);

    lv_layer_t layer;
    lv_canvas_init_layer(ctx->canvas, &layer);

    /* Draw color wheel segments based on animation tick */
    int cx = CANVAS_SIZE / 2;
    int cy = CANVAS_SIZE / 2;
    int r  = CANVAS_SIZE / 2 - 8;

    /* Rotating color bars to show RGB vs YCbCr concept */
    float phase = (float)ctx->anim_tick * 0.05f;

    lv_color_t colors[] = { CS_RED, CS_GREEN, CS_BLUE, CS_YELLOW, CS_CYAN, CS_MAGENTA };
    int num_colors = 6;

    for (int i = 0; i < num_colors; i++) {
        float angle = phase + (float)i * (3.14159f * 2.0f / (float)num_colors);
        int bx = cx + (int)((float)r * 0.7f * cosf(angle));
        int by = cy + (int)((float)r * 0.7f * sinf(angle));

        int size = 14 + (int)(6.0f * sinf(phase + (float)i));

        lv_draw_rect_dsc_t rect_dsc;
        lv_draw_rect_dsc_init(&rect_dsc);
        rect_dsc.bg_color = colors[i];
        rect_dsc.bg_opa   = LV_OPA_70;
        rect_dsc.radius   = 4;

        lv_area_t area = {
            .x1 = bx - size / 2,
            .y1 = by - size / 2,
            .x2 = bx + size / 2,
            .y2 = by + size / 2,
        };
        lv_draw_rect(&layer, &rect_dsc, &area);
    }

    /* Center label indicator */
    lv_draw_rect_dsc_t center_dsc;
    lv_draw_rect_dsc_init(&center_dsc);
    center_dsc.bg_color = lv_color_hex(0x1A1A2E);
    center_dsc.bg_opa   = LV_OPA_COVER;
    center_dsc.radius   = LV_RADIUS_CIRCLE;
    center_dsc.border_width = 2;
    center_dsc.border_color = s_formats[ctx->active_format].accent;

    lv_area_t center_area = {
        .x1 = cx - 16, .y1 = cy - 16,
        .x2 = cx + 16, .y2 = cy + 16,
    };
    lv_draw_rect(&layer, &center_dsc, &center_area);

    lv_canvas_finish_layer(ctx->canvas, &layer);
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.parent = parent;

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_IMAGE " Image Format Explorer");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* แปลงรูปภาพ */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "แปลงรูปภาพ");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 28);

    /* UI Concept banner */
    lv_obj_t *concept = lv_label_create(parent);
    lv_label_set_text(concept, LV_SYMBOL_WARNING " UI Concept - IPC frame share not yet available");
    lv_obj_set_style_text_color(concept, lv_color_hex(0xFF9800), 0);
    lv_obj_align(concept, LV_ALIGN_TOP_RIGHT, -10, 8);

    /* === Format tab buttons === */
    for (int i = 0; i < NUM_FORMATS; i++) {
        s_ctx.tab_btns[i] = lv_btn_create(parent);
        lv_obj_set_size(s_ctx.tab_btns[i], 130, 40);
        lv_obj_align(s_ctx.tab_btns[i], LV_ALIGN_TOP_LEFT,
                     16 + i * 140, 38);
        lv_obj_set_style_bg_color(s_ctx.tab_btns[i],
            (i == 0) ? s_formats[0].accent : UI_COLOR_CARD_BG, 0);
        lv_obj_set_style_radius(s_ctx.tab_btns[i], 8, 0);
        lv_obj_add_event_cb(s_ctx.tab_btns[i], tab_btn_cb,
                            LV_EVENT_CLICKED, &s_ctx);

        char lbl_buf[16];
        snprintf(lbl_buf, sizeof(lbl_buf), "%s %s",
                 s_formats[i].icon, s_formats[i].name);
        s_ctx.tab_labels[i] = lv_label_create(s_ctx.tab_btns[i]);
        lv_label_set_text(s_ctx.tab_labels[i], lbl_buf);
        lv_obj_center(s_ctx.tab_labels[i]);
        lv_obj_set_style_text_color(s_ctx.tab_labels[i],
            (i == 0) ? lv_color_hex(0xFFFFFF) : UI_COLOR_TEXT_DIM, 0);
    }

    /* === Detail panel (left) === */
    lv_obj_t *detail_card = example_card_create(parent, 280, 230, UI_COLOR_CARD_BG);
    lv_obj_align(detail_card, LV_ALIGN_TOP_LEFT, 8, 86);

    s_ctx.format_title = example_label_create(detail_card,
        "", &lv_font_montserrat_20, s_formats[0].accent);
    lv_obj_align(s_ctx.format_title, LV_ALIGN_TOP_LEFT, 0, 0);

    s_ctx.ext_label = example_label_create(detail_card,
        "", &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(s_ctx.ext_label, LV_ALIGN_TOP_LEFT, 0, 24);

    int dy = 44;
    for (int i = 0; i < 7; i++) {
        s_ctx.detail_labels[i] = lv_label_create(detail_card);
        lv_label_set_text(s_ctx.detail_labels[i], "");
        lv_obj_set_style_text_font(s_ctx.detail_labels[i],
                                    &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(s_ctx.detail_labels[i], UI_COLOR_TEXT, 0);
        lv_label_set_long_mode(s_ctx.detail_labels[i], LV_LABEL_LONG_WRAP);
        lv_obj_set_width(s_ctx.detail_labels[i], 252);
        lv_obj_align(s_ctx.detail_labels[i], LV_ALIGN_TOP_LEFT, 0, dy);
        dy += 26;
    }

    /* === Color space visualization (right) === */
    lv_obj_t *viz_card = example_card_create(parent, CANVAS_SIZE + 24,
                                              CANVAS_SIZE + 50, UI_COLOR_CARD_BG);
    lv_obj_align(viz_card, LV_ALIGN_TOP_RIGHT, -8, 86);

    lv_obj_t *viz_title = example_label_create(viz_card,
        "Color Space", &lv_font_montserrat_14, UI_COLOR_PRIMARY);
    lv_obj_align(viz_title, LV_ALIGN_TOP_MID, 0, 0);

    s_ctx.canvas = lv_canvas_create(viz_card);
    lv_canvas_set_buffer(s_ctx.canvas, s_ctx.canvas_buf,
                          CANVAS_SIZE, CANVAS_SIZE, LV_COLOR_FORMAT_RGB565);
    lv_canvas_fill_bg(s_ctx.canvas, lv_color_hex(0x0A0A14), LV_OPA_COVER);
    lv_obj_align(s_ctx.canvas, LV_ALIGN_BOTTOM_MID, 0, 0);

    /* === Compression calculator (bottom-left) === */
    lv_obj_t *calc_card = example_card_create(parent, 240, 140, UI_COLOR_CARD_BG);
    lv_obj_align(calc_card, LV_ALIGN_BOTTOM_LEFT, 8, -8);

    lv_obj_t *calc_title = example_label_create(calc_card,
        "Size Calculator", &lv_font_montserrat_14, UI_COLOR_PRIMARY);
    lv_obj_align(calc_title, LV_ALIGN_TOP_LEFT, 0, 0);

    /* Width input */
    lv_obj_t *w_lbl = example_label_create(calc_card,
        "W:", &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_align(w_lbl, LV_ALIGN_TOP_LEFT, 0, 24);

    s_ctx.calc_width_ta = lv_textarea_create(calc_card);
    lv_obj_set_size(s_ctx.calc_width_ta, 70, 32);
    lv_obj_align(s_ctx.calc_width_ta, LV_ALIGN_TOP_LEFT, 24, 20);
    lv_textarea_set_text(s_ctx.calc_width_ta, "640");
    lv_textarea_set_one_line(s_ctx.calc_width_ta, true);
    lv_textarea_set_accepted_chars(s_ctx.calc_width_ta, "0123456789");

    /* Height input */
    lv_obj_t *h_lbl = example_label_create(calc_card,
        "H:", &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_align(h_lbl, LV_ALIGN_TOP_LEFT, 104, 24);

    s_ctx.calc_height_ta = lv_textarea_create(calc_card);
    lv_obj_set_size(s_ctx.calc_height_ta, 70, 32);
    lv_obj_align(s_ctx.calc_height_ta, LV_ALIGN_TOP_LEFT, 128, 20);
    lv_textarea_set_text(s_ctx.calc_height_ta, "480");
    lv_textarea_set_one_line(s_ctx.calc_height_ta, true);
    lv_textarea_set_accepted_chars(s_ctx.calc_height_ta, "0123456789");

    /* Calculate button */
    lv_obj_t *calc_btn = lv_btn_create(calc_card);
    lv_obj_set_size(calc_btn, 200, 32);
    lv_obj_align(calc_btn, LV_ALIGN_TOP_LEFT, 0, 56);
    lv_obj_set_style_bg_color(calc_btn, UI_COLOR_INFO, 0);
    lv_obj_set_style_radius(calc_btn, 6, 0);
    lv_obj_add_event_cb(calc_btn, calc_cb, LV_EVENT_CLICKED, &s_ctx);

    lv_obj_t *calc_btn_lbl = lv_label_create(calc_btn);
    lv_label_set_text(calc_btn_lbl, "Calculate Sizes");
    lv_obj_center(calc_btn_lbl);

    /* Result label */
    s_ctx.calc_result = lv_label_create(calc_card);
    lv_label_set_text(s_ctx.calc_result, "Enter dimensions");
    lv_obj_set_style_text_font(s_ctx.calc_result, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_ctx.calc_result, UI_COLOR_TEXT_DIM, 0);
    lv_label_set_long_mode(s_ctx.calc_result, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_ctx.calc_result, 210);
    lv_obj_align(s_ctx.calc_result, LV_ALIGN_TOP_LEFT, 0, 92);

    /* === Format comparison strip (bottom-right) === */
    lv_obj_t *cmp_card = example_card_create(parent, 220, 140,
                                              lv_color_hex(0x0A1628));
    lv_obj_align(cmp_card, LV_ALIGN_BOTTOM_RIGHT, -8, -8);

    lv_obj_t *cmp_title = example_label_create(cmp_card,
        "Quick Comparison", &lv_font_montserrat_14, UI_COLOR_PRIMARY);
    lv_obj_align(cmp_title, LV_ALIGN_TOP_LEFT, 0, 0);

    /* Comparison bars: JPEG vs PNG vs BMP quality/size tradeoff */
    const char *bar_labels[] = { "JPEG", "PNG", "BMP" };
    lv_color_t bar_colors[] = {
        s_formats[0].accent,
        s_formats[1].accent,
        s_formats[2].accent,
    };
    int bar_quality[] = { 70, 100, 100 };   /* quality % */
    int bar_size[]    = { 7, 33, 100 };     /* relative size % */

    for (int i = 0; i < 3; i++) {
        int by = 22 + i * 38;

        lv_obj_t *name = example_label_create(cmp_card,
            bar_labels[i], &lv_font_montserrat_14, bar_colors[i]);
        lv_obj_align(name, LV_ALIGN_TOP_LEFT, 0, by);

        /* Quality bar */
        lv_obj_t *q_bar = lv_bar_create(cmp_card);
        lv_obj_set_size(q_bar, 80, 10);
        lv_obj_align(q_bar, LV_ALIGN_TOP_LEFT, 48, by + 4);
        lv_bar_set_range(q_bar, 0, 100);
        lv_bar_set_value(q_bar, bar_quality[i], LV_ANIM_OFF);
        lv_obj_set_style_bg_color(q_bar, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_bg_color(q_bar, UI_COLOR_SUCCESS,
                                   LV_PART_INDICATOR);

        /* Size bar */
        lv_obj_t *s_bar = lv_bar_create(cmp_card);
        lv_obj_set_size(s_bar, 80, 10);
        lv_obj_align(s_bar, LV_ALIGN_TOP_LEFT, 48, by + 18);
        lv_bar_set_range(s_bar, 0, 100);
        lv_bar_set_value(s_bar, bar_size[i], LV_ANIM_OFF);
        lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_bg_color(s_bar, UI_COLOR_WARNING,
                                   LV_PART_INDICATOR);

        /* Labels for bars */
        lv_obj_t *q_lbl = example_label_create(cmp_card,
            "Q", &lv_font_montserrat_14, UI_COLOR_SUCCESS);
        lv_obj_align(q_lbl, LV_ALIGN_TOP_LEFT, 132, by);

        lv_obj_t *s_lbl = example_label_create(cmp_card,
            "S", &lv_font_montserrat_14, UI_COLOR_WARNING);
        lv_obj_align(s_lbl, LV_ALIGN_TOP_LEFT, 132, by + 14);
    }

    /* Initialize detail view */
    update_details(&s_ctx);

    /* Start color space animation */
    s_ctx.anim_timer = lv_timer_create(anim_timer_cb, UPDATE_MS, &s_ctx);
}
