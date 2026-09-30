/**
 * @file    main_example.c
 * @brief   Face Detection Concept UI - Bounding box visualization demo
 *
 * @description
 *   Face detection concept UI with simulated bounding box visualization,
 *   confidence display, detection log, and inference stats panel. Draws
 *   animated bounding boxes on a dark preview area using LVGL objects.
 *   Demonstrates what a face detection UI looks like on embedded hardware.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "pse84_common.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define PREVIEW_W           360
#define PREVIEW_H           270
#define MAX_FACES           4
#define DETECT_INTERVAL_MS  500
#define MAX_LOG_LINES       6

/* ---------------------------------------------------------------------------
 * Simulated face detection result
 * --------------------------------------------------------------------------- */
typedef struct {
    int16_t x, y, w, h;
    int     confidence;     /* 0..100 */
} face_bbox_t;

typedef struct {
    lv_obj_t    *parent;
    lv_obj_t    *preview;
    lv_obj_t    *bbox_objs[MAX_FACES];
    lv_obj_t    *conf_labels[MAX_FACES];
    lv_obj_t    *face_count_lbl;
    lv_obj_t    *infer_time_lbl;
    lv_obj_t    *avg_conf_lbl;
    lv_obj_t    *status_label;
    lv_obj_t    *log_label;
    lv_obj_t    *btn_toggle;
    lv_obj_t    *btn_label;
    lv_timer_t  *detect_timer;
    uint32_t     detect_count;
    bool         running;
    char         log_buf[384];
} face_detect_ctx_t;

static face_detect_ctx_t s_ctx;

/* Simple PRNG */
static uint32_t s_rng = 12345;
static uint32_t rng_next(void)
{
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 17;
    s_rng ^= s_rng << 5;
    return s_rng;
}

/* ---------------------------------------------------------------------------
 * Append to detection log
 * --------------------------------------------------------------------------- */
static void log_append(face_detect_ctx_t *ctx, const char *line)
{
    int nl_count = 0;
    size_t buf_len = strlen(ctx->log_buf);
    for (size_t i = 0; i < buf_len; i++) {
        if (ctx->log_buf[i] == '\n') nl_count++;
    }

    while (nl_count >= MAX_LOG_LINES) {
        char *nl = strchr(ctx->log_buf, '\n');
        if (!nl) break;
        size_t rm = (size_t)(nl - ctx->log_buf) + 1;
        memmove(ctx->log_buf, ctx->log_buf + rm, buf_len - rm + 1);
        buf_len -= rm;
        nl_count--;
    }

    if (buf_len + strlen(line) + 2 < sizeof(ctx->log_buf)) {
        if (buf_len > 0) strcat(ctx->log_buf, "\n");
        strcat(ctx->log_buf, line);
    }
    lv_label_set_text(ctx->log_label, ctx->log_buf);
}

/* ---------------------------------------------------------------------------
 * Simulate detection and update bounding boxes
 * --------------------------------------------------------------------------- */
static void detect_timer_cb(lv_timer_t *timer)
{
    face_detect_ctx_t *ctx = (face_detect_ctx_t *)lv_timer_get_user_data(timer);
    if (!ctx->running) return;

    ctx->detect_count++;

    /* Simulate 0..MAX_FACES detections */
    int num_faces = (int)(rng_next() % (MAX_FACES + 1));
    /* Bias toward 1-2 faces */
    if (num_faces > 2 && (rng_next() % 3) != 0) num_faces = 1 + (int)(rng_next() % 2);

    int conf_sum = 0;
    uint32_t infer_ms = 15 + rng_next() % 25;  /* 15-40 ms simulated */

    for (int i = 0; i < MAX_FACES; i++) {
        if (i < num_faces) {
            /* Generate bounding box within preview area */
            int bw = 50 + (int)(rng_next() % 60);
            int bh = 60 + (int)(rng_next() % 70);
            int bx = (int)(rng_next() % (uint32_t)(PREVIEW_W - bw - 10)) + 5;
            int by = (int)(rng_next() % (uint32_t)(PREVIEW_H - bh - 10)) + 5;
            int conf = 55 + (int)(rng_next() % 45);  /* 55-99% */
            conf_sum += conf;

            /* Show and position bbox */
            lv_obj_remove_flag(ctx->bbox_objs[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(ctx->bbox_objs[i], bx, by);
            lv_obj_set_size(ctx->bbox_objs[i], bw, bh);

            /* Color by confidence */
            lv_color_t box_color = (conf > 75) ? UI_COLOR_SUCCESS : UI_COLOR_WARNING;
            lv_obj_set_style_border_color(ctx->bbox_objs[i], box_color, 0);

            /* Confidence label */
            lv_obj_remove_flag(ctx->conf_labels[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(ctx->conf_labels[i], bx, by - 16);

            char conf_str[8];
            snprintf(conf_str, sizeof(conf_str), "%d%%", conf);
            lv_label_set_text(ctx->conf_labels[i], conf_str);
            lv_obj_set_style_text_color(ctx->conf_labels[i], box_color, 0);
        } else {
            /* Hide unused boxes */
            lv_obj_add_flag(ctx->bbox_objs[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ctx->conf_labels[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* Update stats */
    char count_str[24];
    snprintf(count_str, sizeof(count_str), "Faces: %d", num_faces);
    lv_label_set_text(ctx->face_count_lbl, count_str);
    lv_obj_set_style_text_color(ctx->face_count_lbl,
        (num_faces > 0) ? UI_COLOR_SUCCESS : UI_COLOR_TEXT, 0);

    char infer_str[32];
    snprintf(infer_str, sizeof(infer_str), "Infer: %lu ms", (unsigned long)infer_ms);
    lv_label_set_text(ctx->infer_time_lbl, infer_str);

    if (num_faces > 0) {
        char avg_str[24];
        snprintf(avg_str, sizeof(avg_str), "Avg conf: %d%%", conf_sum / num_faces);
        lv_label_set_text(ctx->avg_conf_lbl, avg_str);
    } else {
        lv_label_set_text(ctx->avg_conf_lbl, "Avg conf: --");
    }

    /* Log entry */
    char log_line[64];
    snprintf(log_line, sizeof(log_line), "#%lu: %d face(s), %lu ms",
             (unsigned long)ctx->detect_count, num_faces, (unsigned long)infer_ms);
    log_append(ctx, log_line);
}

/* ---------------------------------------------------------------------------
 * Toggle button callback
 * --------------------------------------------------------------------------- */
static void btn_toggle_cb(lv_event_t *e)
{
    face_detect_ctx_t *ctx = (face_detect_ctx_t *)lv_event_get_user_data(e);

    ctx->running = !ctx->running;

    if (ctx->running) {
        lv_label_set_text(ctx->btn_label, LV_SYMBOL_PAUSE " Stop");
        lv_label_set_text(ctx->status_label, "Detecting...");
        lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_SUCCESS, 0);
        s_rng = (uint32_t)lv_tick_get();
    } else {
        lv_label_set_text(ctx->btn_label, LV_SYMBOL_PLAY " Start");
        lv_label_set_text(ctx->status_label, "Stopped");
        lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_TEXT, 0);

        /* Hide all bounding boxes */
        for (int i = 0; i < MAX_FACES; i++) {
            lv_obj_add_flag(ctx->bbox_objs[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ctx->conf_labels[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
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
    lv_label_set_text(title, LV_SYMBOL_EYE_OPEN " Face Detection");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* ตรวจจับใบหน้า */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "ตรวจจับใบหน้า");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 28);

    /* UI Concept banner */
    lv_obj_t *concept = lv_label_create(parent);
    lv_label_set_text(concept, LV_SYMBOL_WARNING " UI Concept - camera + ML not yet available via IPC");
    lv_obj_set_style_text_color(concept, lv_color_hex(0xFF9800), 0);
    lv_obj_align(concept, LV_ALIGN_TOP_RIGHT, -10, 8);

    /* Status */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Press Start");
    lv_obj_set_style_text_color(s_ctx.status_label, UI_COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 16, 36);

    /* Preview area */
    s_ctx.preview = lv_obj_create(parent);
    lv_obj_set_size(s_ctx.preview, PREVIEW_W, PREVIEW_H);
    lv_obj_align(s_ctx.preview, LV_ALIGN_LEFT_MID, 8, 10);
    lv_obj_set_style_bg_color(s_ctx.preview, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_opa(s_ctx.preview, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_ctx.preview, UI_COLOR_INFO, 0);
    lv_obj_set_style_border_width(s_ctx.preview, 2, 0);
    lv_obj_set_style_radius(s_ctx.preview, 4, 0);
    lv_obj_clear_flag(s_ctx.preview, LV_OBJ_FLAG_SCROLLABLE);

    /* Preview placeholder text */
    lv_obj_t *ph = lv_label_create(s_ctx.preview);
    lv_label_set_text(ph, "Camera Feed");
    lv_obj_set_style_text_color(ph, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(ph, &lv_font_montserrat_16, 0);
    lv_obj_align(ph, LV_ALIGN_CENTER, 0, 0);

    /* Create bounding box objects (hidden initially) */
    for (int i = 0; i < MAX_FACES; i++) {
        s_ctx.bbox_objs[i] = lv_obj_create(s_ctx.preview);
        lv_obj_set_size(s_ctx.bbox_objs[i], 60, 70);
        lv_obj_set_style_bg_opa(s_ctx.bbox_objs[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(s_ctx.bbox_objs[i], UI_COLOR_SUCCESS, 0);
        lv_obj_set_style_border_width(s_ctx.bbox_objs[i], 2, 0);
        lv_obj_set_style_radius(s_ctx.bbox_objs[i], 2, 0);
        lv_obj_add_flag(s_ctx.bbox_objs[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_ctx.bbox_objs[i], LV_OBJ_FLAG_SCROLLABLE);

        s_ctx.conf_labels[i] = lv_label_create(s_ctx.preview);
        lv_label_set_text(s_ctx.conf_labels[i], "");
        lv_obj_set_style_text_font(s_ctx.conf_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(s_ctx.conf_labels[i], UI_COLOR_SUCCESS, 0);
        lv_obj_add_flag(s_ctx.conf_labels[i], LV_OBJ_FLAG_HIDDEN);
    }

    /* Stats panel */
    lv_obj_t *stats = example_card_create(parent, 160, 160, UI_COLOR_CARD_BG);
    lv_obj_align(stats, LV_ALIGN_RIGHT_MID, -8, -30);

    lv_obj_t *stats_title = example_label_create(stats,
        "Detection Stats", &lv_font_montserrat_14, UI_COLOR_PRIMARY);
    lv_obj_align(stats_title, LV_ALIGN_TOP_LEFT, 0, 0);

    s_ctx.face_count_lbl = example_label_create(stats,
        "Faces: 0", &lv_font_montserrat_20, UI_COLOR_TEXT);
    lv_obj_align(s_ctx.face_count_lbl, LV_ALIGN_TOP_LEFT, 0, 24);

    s_ctx.infer_time_lbl = example_label_create(stats,
        "Infer: -- ms", &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_align(s_ctx.infer_time_lbl, LV_ALIGN_TOP_LEFT, 0, 52);

    s_ctx.avg_conf_lbl = example_label_create(stats,
        "Avg conf: --", &lv_font_montserrat_14, UI_COLOR_WARNING);
    lv_obj_align(s_ctx.avg_conf_lbl, LV_ALIGN_TOP_LEFT, 0, 72);

    lv_obj_t *model_lbl = example_label_create(stats,
        "Model: SSD Nano", &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(model_lbl, LV_ALIGN_TOP_LEFT, 0, 96);

    lv_obj_t *input_lbl = example_label_create(stats,
        "Input: 160x160", &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(input_lbl, LV_ALIGN_TOP_LEFT, 0, 116);

    /* Detection log (bottom) */
    lv_obj_t *log_card = example_card_create(parent, 560, 80, lv_color_hex(0x0A1628));
    lv_obj_align(log_card, LV_ALIGN_BOTTOM_MID, 0, -52);

    s_ctx.log_label = lv_label_create(log_card);
    lv_label_set_text(s_ctx.log_label, "Detection log...");
    lv_obj_set_style_text_color(s_ctx.log_label, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_text_font(s_ctx.log_label, &lv_font_montserrat_14, 0);
    lv_label_set_long_mode(s_ctx.log_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_ctx.log_label, 530);
    lv_obj_align(s_ctx.log_label, LV_ALIGN_TOP_LEFT, 0, 0);

    /* Toggle button */
    s_ctx.btn_toggle = lv_btn_create(parent);
    lv_obj_set_size(s_ctx.btn_toggle, 140, 44);
    lv_obj_align(s_ctx.btn_toggle, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(s_ctx.btn_toggle, UI_COLOR_INFO, 0);
    lv_obj_set_style_radius(s_ctx.btn_toggle, 8, 0);
    lv_obj_add_event_cb(s_ctx.btn_toggle, btn_toggle_cb, LV_EVENT_CLICKED, &s_ctx);

    s_ctx.btn_label = lv_label_create(s_ctx.btn_toggle);
    lv_label_set_text(s_ctx.btn_label, LV_SYMBOL_PLAY " Start");
    lv_obj_center(s_ctx.btn_label);

    /* Detection timer */
    s_ctx.detect_timer = lv_timer_create(detect_timer_cb, DETECT_INTERVAL_MS, &s_ctx);
}
