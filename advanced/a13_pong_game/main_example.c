/*******************************************************************************
 * A11 - Pong Game (Touch Control)
 *
 * Production-derived Pong game for Developer Hub.
 * Adapted from page_game_pong.c (TESAIoT Game Console).
 *
 * 480x384 arena, 14x90 paddles, 14px ball.
 * AI opponent at 3.2px/frame, progressive ball speed-up.
 * 16ms tick timer for smooth 60fps physics. Score to 10.
 * Touch Up/Down for paddle movement (continuous hold).
 * Game Boy 4-tone palette with CRT scanline overlay.
 *
 * Entry point: void example_main(lv_obj_t *parent)
 *
 *******************************************************************************/

#include "pse84_common.h"
#include "game_common.h"
#include "usb_hid_joystick.h"

/*******************************************************************************
 * Game Constants
 *******************************************************************************/
#define PONG_ARENA_W         480
#define PONG_ARENA_H         384
#define PONG_PADDLE_W        14
#define PONG_PADDLE_H        90
#define PONG_BALL_SIZE       14
#define PONG_TICK_MS         16

/*******************************************************************************
 * Game State
 *******************************************************************************/
typedef struct {
    lv_obj_t *parent;
    lv_obj_t *arena;
    lv_obj_t *score_label;
    lv_obj_t *status_label;
    lv_obj_t *paddle_left;
    lv_obj_t *paddle_right;
    lv_obj_t *ball;
    lv_obj_t *ball_shine;
    lv_timer_t *timer;

    float ball_x;
    float ball_y;
    float ball_vx;
    float ball_vy;
    float left_y;
    float right_y;

    uint32_t score_left;
    uint32_t score_right;
    bool running;
    game_input_state_t prev_input;
} pong_state_t;

static pong_state_t s_pong;
static uint32_t s_pong_best;

/*******************************************************************************
 * Forward Declarations
 *******************************************************************************/
static void pong_start(void);
static void pong_step(void);

/*******************************************************************************
 * Helpers
 *******************************************************************************/
static void pong_clamp_paddles(void)
{
    float max_y = (float)(PONG_ARENA_H - PONG_PADDLE_H);
    if (s_pong.left_y < 0.0f) s_pong.left_y = 0.0f;
    if (s_pong.left_y > max_y) s_pong.left_y = max_y;
    if (s_pong.right_y < 0.0f) s_pong.right_y = 0.0f;
    if (s_pong.right_y > max_y) s_pong.right_y = max_y;
}

static void pong_render(void)
{
    lv_obj_set_pos(s_pong.paddle_left, 14, (int32_t)s_pong.left_y);
    lv_obj_set_pos(s_pong.paddle_right,
                   PONG_ARENA_W - 14 - PONG_PADDLE_W, (int32_t)s_pong.right_y);
    lv_obj_set_pos(s_pong.ball, (int32_t)s_pong.ball_x, (int32_t)s_pong.ball_y);
}

static void pong_update_hud(void)
{
    if (s_pong.score_label == NULL) return;
    uint32_t local_best = (s_pong.score_left > s_pong.score_right)
                           ? s_pong.score_left : s_pong.score_right;
    if (local_best > s_pong_best) s_pong_best = local_best;

    lv_label_set_text_fmt(s_pong.score_label,
        "Player: %lu    AI: %lu    Best: %lu",
        (unsigned long)s_pong.score_left,
        (unsigned long)s_pong.score_right,
        (unsigned long)s_pong_best);
}

static void pong_reset_ball(bool to_right)
{
    s_pong.ball_x = (float)(PONG_ARENA_W / 2 - PONG_BALL_SIZE / 2);
    s_pong.ball_y = (float)(PONG_ARENA_H / 2 - PONG_BALL_SIZE / 2);
    s_pong.ball_vx = to_right ? 4.4f : -4.4f;
    s_pong.ball_vy = (float)((int32_t)lv_rand(-18, 18)) / 10.0f;
}

static bool pong_overlap(float ax, float ay, float aw, float ah,
                          float bx, float by, float bw, float bh)
{
    if (ax + aw < bx) return false;
    if (ax > bx + bw) return false;
    if (ay + ah < by) return false;
    if (ay > by + bh) return false;
    return true;
}

/*******************************************************************************
 * Game Over
 *******************************************************************************/
static void pong_game_over(const char *msg)
{
    s_pong.running = false;
    lv_label_set_text_fmt(s_pong.status_label,
        "%s  |  Tap " LV_SYMBOL_REFRESH " to Restart", msg);
    pong_update_hud();
}

/*******************************************************************************
 * Start
 *******************************************************************************/
static void pong_start(void)
{
    s_pong.left_y = (float)(PONG_ARENA_H / 2 - PONG_PADDLE_H / 2);
    s_pong.right_y = s_pong.left_y;
    s_pong.score_left = 0;
    s_pong.score_right = 0;
    s_pong.running = true;

    pong_reset_ball(true);
    pong_clamp_paddles();
    pong_render();

    lv_label_set_text(s_pong.status_label,
        "UP/DOWN to move paddle. First to 10 wins!");
    pong_update_hud();
}

/*******************************************************************************
 * Game Step
 *******************************************************************************/
static void pong_step(void)
{
    float ball_center_y;

    s_pong.ball_x += s_pong.ball_vx;
    s_pong.ball_y += s_pong.ball_vy;

    /* Top/bottom wall bounce */
    if (s_pong.ball_y <= 0.0f) {
        s_pong.ball_y = 0.0f;
        s_pong.ball_vy = -s_pong.ball_vy;
    }
    if (s_pong.ball_y >= (float)(PONG_ARENA_H - PONG_BALL_SIZE)) {
        s_pong.ball_y = (float)(PONG_ARENA_H - PONG_BALL_SIZE);
        s_pong.ball_vy = -s_pong.ball_vy;
    }

    /* Left paddle collision - deflection angle based on hit position */
    if (pong_overlap(s_pong.ball_x, s_pong.ball_y,
                     PONG_BALL_SIZE, PONG_BALL_SIZE,
                     14.0f, s_pong.left_y,
                     PONG_PADDLE_W, PONG_PADDLE_H) &&
        s_pong.ball_vx < 0.0f) {
        float paddle_center = s_pong.left_y + (float)PONG_PADDLE_H * 0.5f;
        float ball_center = s_pong.ball_y + (float)PONG_BALL_SIZE * 0.5f;
        float norm = (ball_center - paddle_center) /
                     ((float)PONG_PADDLE_H * 0.5f);
        s_pong.ball_x = 14.0f + (float)PONG_PADDLE_W;
        s_pong.ball_vx = fabsf(s_pong.ball_vx) + 0.15f;  /* speed up */
        s_pong.ball_vy += norm * 1.4f;
    }

    /* Right paddle collision */
    if (pong_overlap(s_pong.ball_x, s_pong.ball_y,
                     PONG_BALL_SIZE, PONG_BALL_SIZE,
                     (float)(PONG_ARENA_W - 14 - PONG_PADDLE_W), s_pong.right_y,
                     PONG_PADDLE_W, PONG_PADDLE_H) &&
        s_pong.ball_vx > 0.0f) {
        float paddle_center = s_pong.right_y + (float)PONG_PADDLE_H * 0.5f;
        float ball_center = s_pong.ball_y + (float)PONG_BALL_SIZE * 0.5f;
        float norm = (ball_center - paddle_center) /
                     ((float)PONG_PADDLE_H * 0.5f);
        s_pong.ball_x = (float)(PONG_ARENA_W - 14 - PONG_PADDLE_W -
                                 PONG_BALL_SIZE);
        s_pong.ball_vx = -fabsf(s_pong.ball_vx) - 0.12f;  /* speed up */
        s_pong.ball_vy += norm * 1.2f;
    }

    /* Scoring: ball past left edge */
    if (s_pong.ball_x < -25.0f) {
        s_pong.score_right++;
        pong_update_hud();
        if (s_pong.score_right >= 10) {
            pong_game_over("AI wins 10 points");
            return;
        }
        pong_reset_ball(true);
    }

    /* Scoring: ball past right edge */
    if (s_pong.ball_x > (float)(PONG_ARENA_W + 25)) {
        s_pong.score_left++;
        pong_update_hud();
        if (s_pong.score_left >= 10) {
            pong_game_over("Player wins 10 points!");
            return;
        }
        pong_reset_ball(false);
    }

    /* AI opponent (right paddle) - tracks ball at 3.2px/frame */
    ball_center_y = s_pong.ball_y + (float)PONG_BALL_SIZE * 0.5f;
    if (ball_center_y > (s_pong.right_y + (float)PONG_PADDLE_H * 0.5f + 5.0f)) {
        s_pong.right_y += 3.2f;
    } else if (ball_center_y < (s_pong.right_y + (float)PONG_PADDLE_H * 0.5f - 5.0f)) {
        s_pong.right_y -= 3.2f;
    }

    pong_clamp_paddles();
    pong_render();
}

/*******************************************************************************
 * Input Handling (touch Up/Down - continuous hold)
 *******************************************************************************/
static void pong_process_input(void)
{
    game_input_state_t input;
    float move_delta = 0.0f;

    game_input_read(&input);

    if ((input.restart && !s_pong.prev_input.restart) ||
        (!s_pong.running && input.action && !s_pong.prev_input.action)) {
        pong_start();
    }

    if (s_pong.running) {
        /* Continuous hold for smooth paddle movement */
        if (input.up) move_delta -= 7.0f;
        if (input.down) move_delta += 7.0f;

        if (move_delta != 0.0f) {
            s_pong.left_y += move_delta;
            pong_clamp_paddles();
            pong_render();
        }
    }

    s_pong.prev_input = input;
}

/*******************************************************************************
 * Timer Callback
 *******************************************************************************/
static void pong_tick_cb(lv_timer_t *timer)
{
    (void)timer;
    pong_process_input();
    if (s_pong.running) pong_step();
}

/*******************************************************************************
 * Example Entry Point
 *******************************************************************************/
void example_main(lv_obj_t *parent)
{
    lv_obj_t *arena, *mid_line;

    uint32_t saved_best = s_pong_best;
    memset(&s_pong, 0, sizeof(s_pong));
    s_pong_best = saved_best;
    s_pong.parent = parent;

    /* Request USB HID joystick init (F310 support) */
    usb_hid_joystick_request_init();

    /* Dark background on parent */
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    /* Title label */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Pong");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00BCD4), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    /* เกมปิงปอง */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "เกมปิงปอง");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 30);

    /* Score */
    s_pong.score_label = lv_label_create(parent);
    lv_obj_set_style_text_color(s_pong.score_label, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_text_font(s_pong.score_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(s_pong.score_label, 16, 36);

    /* Arena (centered) */
    arena = lv_obj_create(parent);
    lv_obj_set_size(arena, PONG_ARENA_W, PONG_ARENA_H);
    lv_obj_align(arena, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_pad_all(arena, 0, 0);
    lv_obj_set_style_radius(arena, 6, 0);
    lv_obj_set_style_border_width(arena, 2, 0);
    lv_obj_set_style_border_color(arena, gb_color(GB_DARKEST), 0);
    lv_obj_set_style_bg_color(arena, gb_color(GB_LIGHT), 0);
    lv_obj_set_style_shadow_width(arena, 0, 0);
    lv_obj_clear_flag(arena, LV_OBJ_FLAG_SCROLLABLE);
    s_pong.arena = arena;

    /* CRT scanlines */
    game_add_lcd_scanlines(arena, PONG_ARENA_W, PONG_ARENA_H);

    /* Center line */
    mid_line = lv_obj_create(arena);
    lv_obj_set_size(mid_line, 4, PONG_ARENA_H - 20);
    lv_obj_align(mid_line, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(mid_line, gb_color(GB_DARK), 0);
    lv_obj_set_style_border_width(mid_line, 0, 0);
    lv_obj_clear_flag(mid_line, LV_OBJ_FLAG_SCROLLABLE);

    /* Left paddle (player) */
    s_pong.paddle_left = lv_obj_create(arena);
    lv_obj_set_size(s_pong.paddle_left, PONG_PADDLE_W, PONG_PADDLE_H);
    lv_obj_set_style_bg_color(s_pong.paddle_left, gb_color(GB_DARKEST), 0);
    lv_obj_set_style_border_color(s_pong.paddle_left, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_border_width(s_pong.paddle_left, 1, 0);
    lv_obj_set_style_radius(s_pong.paddle_left, 1, 0);
    lv_obj_clear_flag(s_pong.paddle_left, LV_OBJ_FLAG_SCROLLABLE);

    /* Right paddle (AI) */
    s_pong.paddle_right = lv_obj_create(arena);
    lv_obj_set_size(s_pong.paddle_right, PONG_PADDLE_W, PONG_PADDLE_H);
    lv_obj_set_style_bg_color(s_pong.paddle_right, gb_color(GB_DARK), 0);
    lv_obj_set_style_border_color(s_pong.paddle_right, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_border_width(s_pong.paddle_right, 1, 0);
    lv_obj_set_style_radius(s_pong.paddle_right, 1, 0);
    lv_obj_clear_flag(s_pong.paddle_right, LV_OBJ_FLAG_SCROLLABLE);

    /* Ball */
    s_pong.ball = lv_obj_create(arena);
    lv_obj_set_size(s_pong.ball, PONG_BALL_SIZE, PONG_BALL_SIZE);
    lv_obj_set_style_radius(s_pong.ball, PONG_BALL_SIZE / 2, 0);
    lv_obj_set_style_bg_color(s_pong.ball, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_border_color(s_pong.ball, gb_color(GB_DARKEST), 0);
    lv_obj_set_style_border_width(s_pong.ball, 1, 0);
    lv_obj_clear_flag(s_pong.ball, LV_OBJ_FLAG_SCROLLABLE);

    /* Ball shine */
    s_pong.ball_shine = lv_obj_create(s_pong.ball);
    lv_obj_set_size(s_pong.ball_shine, 4, 4);
    lv_obj_set_pos(s_pong.ball_shine, 3, 3);
    lv_obj_set_style_bg_color(s_pong.ball_shine, gb_color(GB_DARK), 0);
    lv_obj_set_style_border_width(s_pong.ball_shine, 0, 0);
    lv_obj_set_style_radius(s_pong.ball_shine, 2, 0);
    lv_obj_clear_flag(s_pong.ball_shine, LV_OBJ_FLAG_SCROLLABLE);

    /* Status label overlaid on arena */
    s_pong.status_label = lv_label_create(arena);
    lv_obj_set_width(s_pong.status_label, PONG_ARENA_W - 20);
    lv_obj_set_style_text_color(s_pong.status_label, gb_color(GB_DARKEST), 0);
    lv_obj_set_style_text_font(s_pong.status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(s_pong.status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_pong.status_label, LV_ALIGN_BOTTOM_MID, 0, -4);

    /* Touch overlay controls: Up/Down D-pad + Restart */
    lv_obj_update_layout(parent);
    lv_coord_t ax = lv_obj_get_x(arena);
    lv_coord_t ay = lv_obj_get_y(arena);
    game_create_touch_dpad(parent, ax, ay, PONG_ARENA_H, false);
    game_create_touch_restart(parent);

    /* Timer + Start */
    s_pong.timer = lv_timer_create(pong_tick_cb, PONG_TICK_MS, NULL);
    pong_start();
}
