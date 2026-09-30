/**
 * @file    main_example.c
 * @brief   Camera Preview Placeholder - UI mockup for camera viewfinder
 *
 * @description
 *   Camera preview placeholder UI demonstrating what a viewfinder interface
 *   looks like: resolution selector dropdown, simulated FPS counter, capture
 *   button with counter, and a gradient-filled frame placeholder. Uses
 *   lv_timer for periodic FPS updates and frame counter animation.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "pse84_common.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define PREVIEW_W           320
#define PREVIEW_H           240
#define FPS_UPDATE_MS       500

/* Resolution options */
static const char *RES_OPTIONS = "320x240 QVGA\n640x480 VGA\n800x600 SVGA\n1280x720 HD";
static const uint16_t RES_W[] = { 320, 640, 800, 1280 };
static const uint16_t RES_H[] = { 240, 480, 600, 720 };

/* ---------------------------------------------------------------------------
 * Context
 * --------------------------------------------------------------------------- */
typedef struct {
    lv_obj_t    *parent;
    lv_obj_t    *preview_area;
    lv_obj_t    *crosshair_h;
    lv_obj_t    *crosshair_v;
    lv_obj_t    *fps_label;
    lv_obj_t    *res_label;
    lv_obj_t    *status_label;
    lv_obj_t    *capture_count_label;
    lv_obj_t    *dropdown;
    lv_obj_t    *btn_capture;
    lv_obj_t    *btn_toggle;
    lv_obj_t    *btn_toggle_label;
    lv_timer_t  *fps_timer;
    uint32_t     capture_count;
    uint32_t     frame_counter;
    bool         streaming;
    uint32_t     selected_res;
} camera_ctx_t;

static camera_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * FPS simulation timer
 * --------------------------------------------------------------------------- */
static void fps_timer_cb(lv_timer_t *timer)
{
    camera_ctx_t *ctx = (camera_ctx_t *)lv_timer_get_user_data(timer);

    if (!ctx->streaming) return;

    ctx->frame_counter++;

    /* Simulate FPS based on resolution (higher res = lower FPS) */
    float sim_fps;
    switch (ctx->selected_res) {
        case 0:  sim_fps = 29.5f + (float)(ctx->frame_counter % 5) * 0.2f; break;
        case 1:  sim_fps = 24.0f + (float)(ctx->frame_counter % 4) * 0.3f; break;
        case 2:  sim_fps = 18.0f + (float)(ctx->frame_counter % 3) * 0.5f; break;
        case 3:  sim_fps = 12.0f + (float)(ctx->frame_counter % 6) * 0.4f; break;
        default: sim_fps = 30.0f; break;
    }

    char fps_str[32];
    snprintf(fps_str, sizeof(fps_str), "FPS: %.1f  Frame: %lu",
             sim_fps, (unsigned long)ctx->frame_counter);
    lv_label_set_text(ctx->fps_label, fps_str);

    /* Animate crosshair slightly to show "live" feel */
    int offset_x = (int)(ctx->frame_counter % 7) - 3;
    int offset_y = (int)(ctx->frame_counter % 5) - 2;
    lv_obj_align(ctx->crosshair_h, LV_ALIGN_CENTER, offset_x, 0);
    lv_obj_align(ctx->crosshair_v, LV_ALIGN_CENTER, 0, offset_y);
}

/* ---------------------------------------------------------------------------
 * Capture button callback
 * --------------------------------------------------------------------------- */
static void btn_capture_cb(lv_event_t *e)
{
    camera_ctx_t *ctx = (camera_ctx_t *)lv_event_get_user_data(e);

    ctx->capture_count++;

    char buf[32];
    snprintf(buf, sizeof(buf), "Captures: %lu", (unsigned long)ctx->capture_count);
    lv_label_set_text(ctx->capture_count_label, buf);

    lv_label_set_text(ctx->status_label, "Frame captured!");
    lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_SUCCESS, 0);
}

/* ---------------------------------------------------------------------------
 * Toggle streaming button callback
 * --------------------------------------------------------------------------- */
static void btn_toggle_cb(lv_event_t *e)
{
    camera_ctx_t *ctx = (camera_ctx_t *)lv_event_get_user_data(e);

    ctx->streaming = !ctx->streaming;

    if (ctx->streaming) {
        lv_label_set_text(ctx->btn_toggle_label, LV_SYMBOL_PAUSE " Stop");
        lv_label_set_text(ctx->status_label, "Streaming...");
        lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_SUCCESS, 0);
        ctx->frame_counter = 0;
    } else {
        lv_label_set_text(ctx->btn_toggle_label, LV_SYMBOL_PLAY " Start");
        lv_label_set_text(ctx->status_label, "Stopped");
        lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_TEXT, 0);
        lv_label_set_text(ctx->fps_label, "FPS: --");
    }
}

/* ---------------------------------------------------------------------------
 * Resolution dropdown callback
 * --------------------------------------------------------------------------- */
static void dropdown_cb(lv_event_t *e)
{
    camera_ctx_t *ctx = (camera_ctx_t *)lv_event_get_user_data(e);
    ctx->selected_res = lv_dropdown_get_selected(ctx->dropdown);

    char buf[32];
    snprintf(buf, sizeof(buf), "%ux%u RGB565",
             RES_W[ctx->selected_res], RES_H[ctx->selected_res]);
    lv_label_set_text(ctx->res_label, buf);
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
    lv_label_set_text(title, LV_SYMBOL_IMAGE " Camera Preview");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* ดูภาพจากกล้อง */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "ดูภาพจากกล้อง");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 28);

    /* UI Concept banner */
    lv_obj_t *concept = lv_label_create(parent);
    lv_label_set_text(concept, LV_SYMBOL_WARNING " UI Concept - camera hardware not yet available via IPC");
    lv_obj_set_style_text_color(concept, lv_color_hex(0xFF9800), 0);
    lv_obj_align(concept, LV_ALIGN_TOP_RIGHT, -10, 8);

    /* Status */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Press Start to begin");
    lv_obj_set_style_text_color(s_ctx.status_label, UI_COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 16, 36);

    /* FPS counter */
    s_ctx.fps_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.fps_label, "FPS: --");
    lv_obj_set_style_text_color(s_ctx.fps_label, UI_COLOR_SUCCESS, 0);
    lv_obj_align(s_ctx.fps_label, LV_ALIGN_TOP_RIGHT, -16, 36);

    /* Preview area (dark rectangle with crosshair) */
    s_ctx.preview_area = lv_obj_create(parent);
    lv_obj_set_size(s_ctx.preview_area, PREVIEW_W, PREVIEW_H);
    lv_obj_align(s_ctx.preview_area, LV_ALIGN_CENTER, -60, 0);
    lv_obj_set_style_bg_color(s_ctx.preview_area, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_opa(s_ctx.preview_area, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_ctx.preview_area, UI_COLOR_INFO, 0);
    lv_obj_set_style_border_width(s_ctx.preview_area, 2, 0);
    lv_obj_set_style_radius(s_ctx.preview_area, 4, 0);
    lv_obj_clear_flag(s_ctx.preview_area, LV_OBJ_FLAG_SCROLLABLE);

    /* Center crosshair (horizontal line) */
    s_ctx.crosshair_h = lv_obj_create(s_ctx.preview_area);
    lv_obj_set_size(s_ctx.crosshair_h, 40, 1);
    lv_obj_set_style_bg_color(s_ctx.crosshair_h, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_bg_opa(s_ctx.crosshair_h, LV_OPA_70, 0);
    lv_obj_set_style_border_width(s_ctx.crosshair_h, 0, 0);
    lv_obj_align(s_ctx.crosshair_h, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(s_ctx.crosshair_h, LV_OBJ_FLAG_SCROLLABLE);

    /* Center crosshair (vertical line) */
    s_ctx.crosshair_v = lv_obj_create(s_ctx.preview_area);
    lv_obj_set_size(s_ctx.crosshair_v, 1, 40);
    lv_obj_set_style_bg_color(s_ctx.crosshair_v, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_bg_opa(s_ctx.crosshair_v, LV_OPA_70, 0);
    lv_obj_set_style_border_width(s_ctx.crosshair_v, 0, 0);
    lv_obj_align(s_ctx.crosshair_v, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(s_ctx.crosshair_v, LV_OBJ_FLAG_SCROLLABLE);

    /* Camera icon in center of preview */
    lv_obj_t *cam_icon = lv_label_create(s_ctx.preview_area);
    lv_label_set_text(cam_icon, LV_SYMBOL_IMAGE);
    lv_obj_set_style_text_font(cam_icon, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(cam_icon, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(cam_icon, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t *cam_text = lv_label_create(s_ctx.preview_area);
    lv_label_set_text(cam_text, "Camera Feed");
    lv_obj_set_style_text_color(cam_text, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(cam_text, LV_ALIGN_CENTER, 0, 40);

    /* Info panel (right side) */
    lv_obj_t *info = example_card_create(parent, 180, 200, UI_COLOR_CARD_BG);
    lv_obj_align(info, LV_ALIGN_RIGHT_MID, -8, 0);

    lv_obj_t *info_title = example_label_create(info,
        "Settings", &lv_font_montserrat_16, UI_COLOR_PRIMARY);
    lv_obj_align(info_title, LV_ALIGN_TOP_LEFT, 0, 0);

    /* Resolution dropdown */
    s_ctx.dropdown = lv_dropdown_create(info);
    lv_obj_set_width(s_ctx.dropdown, 152);
    lv_obj_align(s_ctx.dropdown, LV_ALIGN_TOP_LEFT, 0, 26);
    lv_dropdown_set_options(s_ctx.dropdown, RES_OPTIONS);
    lv_obj_add_event_cb(s_ctx.dropdown, dropdown_cb, LV_EVENT_VALUE_CHANGED, &s_ctx);

    /* Resolution display */
    s_ctx.res_label = example_label_create(info,
        "320x240 RGB565", &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_align(s_ctx.res_label, LV_ALIGN_TOP_LEFT, 0, 72);

    /* Capture count */
    s_ctx.capture_count_label = example_label_create(info,
        "Captures: 0", &lv_font_montserrat_14, UI_COLOR_WARNING);
    lv_obj_align(s_ctx.capture_count_label, LV_ALIGN_TOP_LEFT, 0, 96);

    /* Buffer size info */
    lv_obj_t *buf_lbl = example_label_create(info,
        "Buffer: 150 KB", &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(buf_lbl, LV_ALIGN_TOP_LEFT, 0, 120);

    lv_obj_t *fmt_lbl = example_label_create(info,
        "Format: RGB565", &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(fmt_lbl, LV_ALIGN_TOP_LEFT, 0, 140);

    /* Buttons row */
    s_ctx.btn_toggle = lv_btn_create(parent);
    lv_obj_set_size(s_ctx.btn_toggle, 140, 44);
    lv_obj_align(s_ctx.btn_toggle, LV_ALIGN_BOTTOM_MID, -80, -8);
    lv_obj_set_style_bg_color(s_ctx.btn_toggle, UI_COLOR_INFO, 0);
    lv_obj_set_style_radius(s_ctx.btn_toggle, 8, 0);
    lv_obj_add_event_cb(s_ctx.btn_toggle, btn_toggle_cb, LV_EVENT_CLICKED, &s_ctx);

    s_ctx.btn_toggle_label = lv_label_create(s_ctx.btn_toggle);
    lv_label_set_text(s_ctx.btn_toggle_label, LV_SYMBOL_PLAY " Start");
    lv_obj_center(s_ctx.btn_toggle_label);

    s_ctx.btn_capture = lv_btn_create(parent);
    lv_obj_set_size(s_ctx.btn_capture, 140, 44);
    lv_obj_align(s_ctx.btn_capture, LV_ALIGN_BOTTOM_MID, 80, -8);
    lv_obj_set_style_bg_color(s_ctx.btn_capture, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_radius(s_ctx.btn_capture, 8, 0);
    lv_obj_add_event_cb(s_ctx.btn_capture, btn_capture_cb, LV_EVENT_CLICKED, &s_ctx);

    lv_obj_t *cap_lbl = lv_label_create(s_ctx.btn_capture);
    lv_label_set_text(cap_lbl, LV_SYMBOL_IMAGE " Capture");
    lv_obj_center(cap_lbl);

    /* FPS simulation timer */
    s_ctx.fps_timer = lv_timer_create(fps_timer_cb, FPS_UPDATE_MS, &s_ctx);
}
