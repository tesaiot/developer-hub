/**
 * @file    main_example.c
 * @brief   Joystick - F310 Controller with Live USB HID Overlay
 *
 * @description
 *   Production joystick visualization from page_joystick.c.
 *   Displays the Logitech F310 controller image (420x260 RGB565)
 *   with real-time overlay indicators:
 *     - 10 button dots (X/A/B/Y/LB/RB/Back/Start/L3/R3)
 *     - 2 analog stick position dots (left/right)
 *     - 4 D-pad direction dots (Up/Down/Left/Right)
 *     - Button state table (7 rows × 4 cols)
 *     - USB connection status + VID/PID display
 *
 *   Joystick data is LOCAL on CM55 (USB Host). No IPC needed.
 *   usb_hid_joystick_get_state() returns live F310 HID report.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "pse84_common.h"
#include "usb_hid_joystick.h"
#include "img_f310.h"

/* ---------------------------------------------------------------------------
 * Button overlay constants (from production page_joystick.c)
 * --------------------------------------------------------------------------- */
#define JOY_BTN_DOT_COUNT  10
#define JOY_DPAD_DOT_COUNT 4
#define BTN_DOT_SIZE       14
#define STICK_DOT_SIZE     14
#define STICK_RANGE        26

enum { DOT_X = 0, DOT_A, DOT_B, DOT_Y, DOT_LB, DOT_RB, DOT_BACK, DOT_START, DOT_L3, DOT_R3 };
enum { DPAD_UP = 0, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT };

typedef struct { int16_t x; int16_t y; } btn_pos_t;

/* Button positions on F310 image (pixel coordinates) */
static const btn_pos_t s_btn_pos[JOY_BTN_DOT_COUNT] = {
    [DOT_X]     = { 311,  96 },
    [DOT_A]     = { 338, 121 },
    [DOT_B]     = { 374,  88 },
    [DOT_Y]     = { 346,  62 },
    [DOT_LB]    = {  80,  18 },
    [DOT_RB]    = { 340,  18 },
    [DOT_BACK]  = { 172,  77 },
    [DOT_START] = { 250,  77 },
    [DOT_L3]    = { 148, 163 },
    [DOT_R3]    = { 265, 163 },
};

/* Button colors */
static const uint32_t s_btn_color[JOY_BTN_DOT_COUNT] = {
    [DOT_X]     = 0x2196F3,   /* blue   */
    [DOT_A]     = 0x4CAF50,   /* green  */
    [DOT_B]     = 0xF44336,   /* red    */
    [DOT_Y]     = 0xFF9800,   /* orange */
    [DOT_LB]    = 0x808080,
    [DOT_RB]    = 0x808080,
    [DOT_BACK]  = 0x808080,
    [DOT_START] = 0x808080,
    [DOT_L3]    = 0x00BCD4,   /* cyan   */
    [DOT_R3]    = 0x00BCD4,
};

#define LEFT_STICK_CX    148
#define LEFT_STICK_CY    163
#define RIGHT_STICK_CX   265
#define RIGHT_STICK_CY   163

#define DPAD_DOT_SIZE    12
static const btn_pos_t s_dpad_pos[JOY_DPAD_DOT_COUNT] = {
    [DPAD_UP]    = {  86,  83 },
    [DPAD_DOWN]  = {  86, 117 },
    [DPAD_LEFT]  = {  69, 100 },
    [DPAD_RIGHT] = { 103, 100 },
};

static const uint8_t s_hat_to_dpad[] = {
    (1 << DPAD_UP),
    (1 << DPAD_UP)   | (1 << DPAD_RIGHT),
    (1 << DPAD_RIGHT),
    (1 << DPAD_DOWN) | (1 << DPAD_RIGHT),
    (1 << DPAD_DOWN),
    (1 << DPAD_DOWN) | (1 << DPAD_LEFT),
    (1 << DPAD_LEFT),
    (1 << DPAD_UP)   | (1 << DPAD_LEFT),
    0,  /* neutral */
};

static const char * const s_dpad_dirs[] = {
    "Up", "Up-Right", "Right", "Down-Right",
    "Down", "Down-Left", "Left", "Up-Left", "---"
};

/* ---------------------------------------------------------------------------
 * Module-static context
 * --------------------------------------------------------------------------- */
typedef struct {
    lv_obj_t *left_stick_dot;
    lv_obj_t *right_stick_dot;
    lv_obj_t *btn_dots[JOY_BTN_DOT_COUNT];
    lv_obj_t *dpad_dots[JOY_DPAD_DOT_COUNT];
    lv_obj_t *conn_label;
    lv_obj_t *info_label;
    lv_obj_t *btn_table;
} joy_ctx_t;

static joy_ctx_t s_ctx;
static char s_info_buf[64];

/* ---------------------------------------------------------------------------
 * Helper: create overlay dot on image
 * --------------------------------------------------------------------------- */
static lv_obj_t *create_dot(lv_obj_t *parent, int size, int cx, int cy,
                             uint32_t color, lv_opa_t opa)
{
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_set_size(dot, size, size);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(dot, opa, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_pad_all(dot, 0, 0);
    lv_obj_set_pos(dot, cx - size / 2, cy - size / 2);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return dot;
}

/* ---------------------------------------------------------------------------
 * Table draw callback - color "ON" cells green
 * --------------------------------------------------------------------------- */
static void tbl_draw_cb(lv_event_t *e)
{
    lv_draw_task_t *draw_task = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t *base = (lv_draw_dsc_base_t *)lv_draw_task_get_draw_dsc(draw_task);
    if (!base || base->part != LV_PART_ITEMS) return;

    uint32_t col = base->id2;
    if (col != 1 && col != 3) return;

    lv_obj_t *table = lv_event_get_target(e);
    const char *val = lv_table_get_cell_value(table, base->id1, col);

    lv_draw_label_dsc_t *label_dsc = lv_draw_task_get_label_dsc(draw_task);
    if (label_dsc && val && strcmp(val, "ON") == 0) {
        label_dsc->color = lv_color_hex(0x4CAF50);
        label_dsc->font = &lv_font_montserrat_16;
    }
}

/* ---------------------------------------------------------------------------
 * Create button state table
 * --------------------------------------------------------------------------- */
#define TBL_ROWS 7
#define TBL_COLS 4

static lv_obj_t *create_btn_table(lv_obj_t *parent)
{
    lv_obj_t *table = lv_table_create(parent);
    lv_table_set_column_count(table, TBL_COLS);
    lv_table_set_row_count(table, TBL_ROWS);
    lv_table_set_column_width(table, 0, 56);
    lv_table_set_column_width(table, 1, 67);
    lv_table_set_column_width(table, 2, 56);
    lv_table_set_column_width(table, 3, 67);

    lv_table_set_cell_value(table, 0, 0, "X");
    lv_table_set_cell_value(table, 0, 2, "Y");
    lv_table_set_cell_value(table, 1, 0, "A");
    lv_table_set_cell_value(table, 1, 2, "B");
    lv_table_set_cell_value(table, 2, 0, "LB");
    lv_table_set_cell_value(table, 2, 2, "RB");
    lv_table_set_cell_value(table, 3, 0, "LT");
    lv_table_set_cell_value(table, 3, 2, "RT");
    lv_table_set_cell_value(table, 4, 0, "Back");
    lv_table_set_cell_value(table, 4, 2, "Start");
    lv_table_set_cell_value(table, 5, 0, "L3");
    lv_table_set_cell_value(table, 5, 2, "R3");
    lv_table_set_cell_value(table, 6, 0, "DPad");
    lv_table_set_cell_value(table, 6, 2, "");

    for (int r = 0; r < TBL_ROWS - 1; r++) {
        lv_table_set_cell_value(table, r, 1, "--");
        lv_table_set_cell_value(table, r, 3, "--");
    }
    lv_table_set_cell_value(table, 6, 1, "---");
    lv_table_set_cell_value(table, 6, 3, "");

    /* Styling */
    lv_obj_set_style_bg_color(table, UI_COLOR_CARD_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(table, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(table, lv_color_hex(0xFFD740), LV_PART_MAIN);
    lv_obj_set_style_border_width(table, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(table, UI_CARD_RADIUS, LV_PART_MAIN);
    lv_obj_set_style_pad_all(table, 0, LV_PART_MAIN);

    lv_obj_set_style_text_font(table, &lv_font_montserrat_14, LV_PART_ITEMS);
    lv_obj_set_style_text_color(table, UI_COLOR_TEXT, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(table, UI_COLOR_CARD_BG, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(table, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_border_color(table, lv_color_hex(0x2A3A5C), LV_PART_ITEMS);
    lv_obj_set_style_border_width(table, 1, LV_PART_ITEMS);
    lv_obj_set_style_pad_top(table, 4, LV_PART_ITEMS);
    lv_obj_set_style_pad_bottom(table, 4, LV_PART_ITEMS);
    lv_obj_set_style_pad_left(table, 6, LV_PART_ITEMS);
    lv_obj_set_style_pad_right(table, 4, LV_PART_ITEMS);

    lv_obj_remove_flag(table, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(table, tbl_draw_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(table, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

    return table;
}

/* ---------------------------------------------------------------------------
 * Timer callback - read USB HID and update overlay
 * --------------------------------------------------------------------------- */
static void joy_timer_cb(lv_timer_t *t)
{
    (void)t;
    const joystick_state_t *joy = usb_hid_joystick_get_state();

    if (!joy->connected) {
        if (s_ctx.conn_label) {
            if (joy->usb_init_done) {
                lv_label_set_text(s_ctx.conn_label, "USB Host: OK\nWaiting for\ndevice...");
                lv_obj_set_style_text_color(s_ctx.conn_label, UI_COLOR_WARNING, 0);
            } else {
                lv_label_set_text(s_ctx.conn_label, "USB Host:\nNOT init");
                lv_obj_set_style_text_color(s_ctx.conn_label, UI_COLOR_ERROR, 0);
            }
        }
        /* Dim all dots when disconnected */
        for (int i = 0; i < JOY_BTN_DOT_COUNT; i++)
            if (s_ctx.btn_dots[i]) lv_obj_set_style_bg_opa(s_ctx.btn_dots[i], LV_OPA_20, 0);
        for (int i = 0; i < JOY_DPAD_DOT_COUNT; i++)
            if (s_ctx.dpad_dots[i]) lv_obj_set_style_bg_opa(s_ctx.dpad_dots[i], LV_OPA_20, 0);
        return;
    }

    /* Connected - update everything */
    if (s_ctx.conn_label) {
        lv_label_set_text(s_ctx.conn_label, "Connected");
        lv_obj_set_style_text_color(s_ctx.conn_label, UI_COLOR_SUCCESS, 0);
    }

    uint8_t lx = joy->report.left_x;
    uint8_t ly = joy->report.left_y;
    uint8_t rx = joy->report.right_x;
    uint8_t ry = joy->report.right_y;
    uint8_t b1 = joy->report.buttons1;
    uint8_t b2 = joy->report.buttons2;

    /* Button overlay dots */
    bool pressed[JOY_BTN_DOT_COUNT];
    pressed[DOT_X]     = (b1 & F310_BTN_X)     != 0;
    pressed[DOT_A]     = (b1 & F310_BTN_A)     != 0;
    pressed[DOT_B]     = (b1 & F310_BTN_B)     != 0;
    pressed[DOT_Y]     = (b1 & F310_BTN_Y)     != 0;
    pressed[DOT_LB]    = (b2 & F310_BTN_LB)    != 0;
    pressed[DOT_RB]    = (b2 & F310_BTN_RB)    != 0;
    pressed[DOT_BACK]  = (b2 & F310_BTN_BACK)  != 0;
    pressed[DOT_START] = (b2 & F310_BTN_START) != 0;
    pressed[DOT_L3]    = (b2 & F310_BTN_L3)    != 0;
    pressed[DOT_R3]    = (b2 & F310_BTN_R3)    != 0;

    for (int i = 0; i < JOY_BTN_DOT_COUNT; i++)
        if (s_ctx.btn_dots[i])
            lv_obj_set_style_bg_opa(s_ctx.btn_dots[i],
                                    pressed[i] ? LV_OPA_COVER : LV_OPA_40, 0);

    /* Analog stick positions */
    if (s_ctx.left_stick_dot) {
        int dx = ((int)lx - 128) * STICK_RANGE / 128;
        int dy = ((int)ly - 128) * STICK_RANGE / 128;
        lv_obj_set_pos(s_ctx.left_stick_dot,
                       LEFT_STICK_CX + dx - STICK_DOT_SIZE / 2,
                       LEFT_STICK_CY + dy - STICK_DOT_SIZE / 2);
    }
    if (s_ctx.right_stick_dot) {
        int dx = ((int)rx - 128) * STICK_RANGE / 128;
        int dy = ((int)ry - 128) * STICK_RANGE / 128;
        lv_obj_set_pos(s_ctx.right_stick_dot,
                       RIGHT_STICK_CX + dx - STICK_DOT_SIZE / 2,
                       RIGHT_STICK_CY + dy - STICK_DOT_SIZE / 2);
    }

    /* D-pad */
    uint8_t hat = b1 & F310_HAT_MASK;
    if (hat > F310_HAT_NEUTRAL) hat = F310_HAT_NEUTRAL;
    uint8_t dpad_mask = s_hat_to_dpad[hat];
    for (int i = 0; i < JOY_DPAD_DOT_COUNT; i++)
        if (s_ctx.dpad_dots[i])
            lv_obj_set_style_bg_opa(s_ctx.dpad_dots[i],
                                    (dpad_mask & (1 << i)) ? LV_OPA_COVER : LV_OPA_40, 0);

    /* Button state table */
    if (s_ctx.btn_table) {
        #define ON  "ON"
        #define OFF "--"
        lv_table_set_cell_value(s_ctx.btn_table, 0, 1, pressed[DOT_X]     ? ON : OFF);
        lv_table_set_cell_value(s_ctx.btn_table, 0, 3, pressed[DOT_Y]     ? ON : OFF);
        lv_table_set_cell_value(s_ctx.btn_table, 1, 1, pressed[DOT_A]     ? ON : OFF);
        lv_table_set_cell_value(s_ctx.btn_table, 1, 3, pressed[DOT_B]     ? ON : OFF);
        lv_table_set_cell_value(s_ctx.btn_table, 2, 1, pressed[DOT_LB]    ? ON : OFF);
        lv_table_set_cell_value(s_ctx.btn_table, 2, 3, pressed[DOT_RB]    ? ON : OFF);
        lv_table_set_cell_value(s_ctx.btn_table, 3, 1, (b2 & F310_BTN_LT) ? ON : OFF);
        lv_table_set_cell_value(s_ctx.btn_table, 3, 3, (b2 & F310_BTN_RT) ? ON : OFF);
        lv_table_set_cell_value(s_ctx.btn_table, 4, 1, pressed[DOT_BACK]  ? ON : OFF);
        lv_table_set_cell_value(s_ctx.btn_table, 4, 3, pressed[DOT_START] ? ON : OFF);
        lv_table_set_cell_value(s_ctx.btn_table, 5, 1, pressed[DOT_L3]    ? ON : OFF);
        lv_table_set_cell_value(s_ctx.btn_table, 5, 3, pressed[DOT_R3]    ? ON : OFF);
        lv_table_set_cell_value(s_ctx.btn_table, 6, 1, s_dpad_dirs[hat]);
        #undef ON
        #undef OFF
    }

    if (s_ctx.info_label) {
        snprintf(s_info_buf, sizeof(s_info_buf),
                 "VID:%04X PID:%04X  Mode:%u",
                 joy->vid, joy->pid, (unsigned)joy->report.mode);
        lv_label_set_text(s_ctx.info_label, s_info_buf);
    }
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));

    /* Request USB Host init (non-blocking) */
    (void)usb_hid_joystick_request_init();

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* Title */
    lv_obj_t *title = example_label_create(parent,
        LV_SYMBOL_SETTINGS " F310 Joystick - Live USB HID",
        &lv_font_montserrat_20, lv_color_hex(0xFFD740));
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* จอยสติ๊ก */
    thai_label(parent, "จอยสติ๊ก USB (ฮาร์ดแวร์จริง)", 14, UI_COLOR_TEXT_DIM);

    /* ── LEFT: F310 Image + overlay dots ───────────────────────────── */
    lv_obj_t *img_cont = lv_obj_create(parent);
    lv_obj_set_size(img_cont, IMG_F310_W, IMG_F310_H);
    lv_obj_set_style_pad_all(img_cont, 0, 0);
    lv_obj_set_style_bg_opa(img_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(img_cont, 0, 0);
    lv_obj_clear_flag(img_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(img_cont, LV_ALIGN_LEFT_MID, 16, 15);

    lv_obj_t *img = lv_image_create(img_cont);
    lv_image_set_src(img, &img_f310);
    lv_obj_set_pos(img, 0, 0);

    /* Button overlay dots */
    for (int i = 0; i < JOY_BTN_DOT_COUNT; i++)
        s_ctx.btn_dots[i] = create_dot(img_cont, BTN_DOT_SIZE,
                                       s_btn_pos[i].x, s_btn_pos[i].y,
                                       s_btn_color[i], LV_OPA_40);

    /* Analog stick dots */
    s_ctx.left_stick_dot = create_dot(img_cont, STICK_DOT_SIZE,
                                      LEFT_STICK_CX, LEFT_STICK_CY,
                                      0x00BCD4, LV_OPA_COVER);
    s_ctx.right_stick_dot = create_dot(img_cont, STICK_DOT_SIZE,
                                       RIGHT_STICK_CX, RIGHT_STICK_CY,
                                       0x00BCD4, LV_OPA_COVER);

    /* D-pad dots */
    for (int i = 0; i < JOY_DPAD_DOT_COUNT; i++)
        s_ctx.dpad_dots[i] = create_dot(img_cont, DPAD_DOT_SIZE,
                                         s_dpad_pos[i].x, s_dpad_pos[i].y,
                                         0x00BCD4, LV_OPA_40);

    /* ── RIGHT: Connection status + button table ───────────────────── */
    lv_obj_t *right_col = lv_obj_create(parent);
    lv_obj_remove_style_all(right_col);
    lv_obj_set_size(right_col, 290, 340);
    lv_obj_align(right_col, LV_ALIGN_RIGHT_MID, -16, 15);
    lv_obj_set_flex_flow(right_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(right_col, 8, 0);
    lv_obj_clear_flag(right_col, LV_OBJ_FLAG_SCROLLABLE);

    s_ctx.conn_label = lv_label_create(right_col);
    lv_label_set_text(s_ctx.conn_label, "USB Host:\nNOT init");
    lv_obj_set_style_text_font(s_ctx.conn_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_ctx.conn_label, UI_COLOR_ERROR, 0);

    s_ctx.info_label = lv_label_create(right_col);
    lv_label_set_text(s_ctx.info_label, "VID:---- PID:----");
    lv_obj_set_style_text_font(s_ctx.info_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_ctx.info_label, UI_COLOR_TEXT_DIM, 0);

    s_ctx.btn_table = create_btn_table(right_col);

    /* Start 50ms timer (20 fps joystick polling) */
    lv_timer_create(joy_timer_cb, 50, NULL);
}
