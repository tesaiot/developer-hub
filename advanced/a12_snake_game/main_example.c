/*******************************************************************************
 * A10 - Snake Game (Touch D-pad Control)
 *
 * Production-derived Snake game for Developer Hub.
 * Adapted from page_game_snake.c (TESAIoT Game Console).
 *
 * 30x24 grid, 16px cells, max 140 segments (pre-allocated object pool).
 * 115ms tick timer drives game logic.
 * Touch D-pad for direction (edge-triggered, prevents 180-degree reversal).
 * Head with eyes + tongue (orientation-based).
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
#define SNAKE_GRID_W        30
#define SNAKE_GRID_H        24
#define SNAKE_CELL_PX       16
#define SNAKE_MAX_LEN       140
#define SNAKE_TICK_MS       115

#define SNAKE_ARENA_W       (SNAKE_GRID_W * SNAKE_CELL_PX)  /* 480 */
#define SNAKE_ARENA_H       (SNAKE_GRID_H * SNAKE_CELL_PX)  /* 384 */

/*******************************************************************************
 * Game State
 *******************************************************************************/
typedef struct {
    lv_obj_t *parent;
    lv_obj_t *arena;
    lv_obj_t *score_label;
    lv_obj_t *status_label;
    lv_obj_t *food_obj;
    lv_obj_t *head_eye_l;
    lv_obj_t *head_eye_r;
    lv_obj_t *head_tongue;
    lv_obj_t *segment_objs[SNAKE_MAX_LEN];
    lv_timer_t *timer;

    int16_t snake_x[SNAKE_MAX_LEN];
    int16_t snake_y[SNAKE_MAX_LEN];
    uint16_t snake_len;

    int8_t dir_x;
    int8_t dir_y;
    int8_t next_dir_x;
    int8_t next_dir_y;

    int16_t food_x;
    int16_t food_y;

    uint32_t score;
    bool running;
    game_input_state_t prev_input;
} snake_state_t;

static snake_state_t s_snake;
static uint32_t s_snake_best;

/*******************************************************************************
 * Forward Declarations
 *******************************************************************************/
static void snake_start(void);
static void snake_step(void);
static void snake_render(void);

/*******************************************************************************
 * HUD Update
 *******************************************************************************/
static void snake_update_hud(void)
{
    if (s_snake.score_label == NULL) return;
    lv_label_set_text_fmt(
        s_snake.score_label,
        "Score: %lu    Best: %lu    Len: %u",
        (unsigned long)s_snake.score,
        (unsigned long)s_snake_best,
        (unsigned int)s_snake.snake_len);
}

/*******************************************************************************
 * Collision Detection
 *******************************************************************************/
static bool snake_contains_xy(int16_t x, int16_t y)
{
    uint16_t i;
    for (i = 0; i < s_snake.snake_len; i++) {
        if (s_snake.snake_x[i] == x && s_snake.snake_y[i] == y) {
            return true;
        }
    }
    return false;
}

/*******************************************************************************
 * Food Spawning
 *******************************************************************************/
static void snake_spawn_food(void)
{
    int tries;
    for (tries = 0; tries < 200; tries++) {
        int16_t x = (int16_t)lv_rand(0, SNAKE_GRID_W - 1);
        int16_t y = (int16_t)lv_rand(0, SNAKE_GRID_H - 1);
        if (!snake_contains_xy(x, y)) {
            s_snake.food_x = x;
            s_snake.food_y = y;
            lv_obj_set_pos(s_snake.food_obj,
                           s_snake.food_x * SNAKE_CELL_PX,
                           s_snake.food_y * SNAKE_CELL_PX);
            return;
        }
    }
    s_snake.food_x = 0;
    s_snake.food_y = 0;
    lv_obj_set_pos(s_snake.food_obj, 0, 0);
}

/*******************************************************************************
 * Snake Visual Rendering
 *******************************************************************************/
static void snake_render(void)
{
    uint16_t i;
    int16_t hx, hy;
    int16_t ex1, ey1, ex2, ey2;
    int16_t tx, ty, tw, th;

    for (i = 0; i < SNAKE_MAX_LEN; i++) {
        if (i < s_snake.snake_len) {
            lv_obj_clear_flag(s_snake.segment_objs[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(s_snake.segment_objs[i],
                           s_snake.snake_x[i] * SNAKE_CELL_PX,
                           s_snake.snake_y[i] * SNAKE_CELL_PX);

            if (i == 0) {
                lv_obj_set_style_bg_color(s_snake.segment_objs[i],
                                           gb_color(GB_DARKEST), 0);
                lv_obj_set_style_border_color(s_snake.segment_objs[i],
                                               gb_color(GB_LIGHT), 0);
                lv_obj_set_style_radius(s_snake.segment_objs[i], 4, 0);
            } else {
                lv_obj_set_style_bg_color(s_snake.segment_objs[i],
                                           gb_color(GB_DARK), 0);
                lv_obj_set_style_border_color(s_snake.segment_objs[i],
                                               gb_color(GB_LIGHT), 0);
                lv_obj_set_style_radius(s_snake.segment_objs[i], 2, 0);
            }
        } else {
            lv_obj_add_flag(s_snake.segment_objs[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* Head position */
    hx = (int16_t)(s_snake.snake_x[0] * SNAKE_CELL_PX);
    hy = (int16_t)(s_snake.snake_y[0] * SNAKE_CELL_PX);

    /* Eyes + tongue orientation based on heading */
    if (s_snake.dir_x > 0) {
        ex1 = hx + 10; ey1 = hy + 4;
        ex2 = hx + 10; ey2 = hy + 10;
        tx = hx + 16; ty = hy + 7; tw = 4; th = 2;
    } else if (s_snake.dir_x < 0) {
        ex1 = hx + 3; ey1 = hy + 4;
        ex2 = hx + 3; ey2 = hy + 10;
        tx = hx - 4; ty = hy + 7; tw = 4; th = 2;
    } else if (s_snake.dir_y < 0) {
        ex1 = hx + 4; ey1 = hy + 3;
        ex2 = hx + 10; ey2 = hy + 3;
        tx = hx + 7; ty = hy - 4; tw = 2; th = 4;
    } else {
        ex1 = hx + 4; ey1 = hy + 10;
        ex2 = hx + 10; ey2 = hy + 10;
        tx = hx + 7; ty = hy + 16; tw = 2; th = 4;
    }

    lv_obj_set_pos(s_snake.head_eye_l, ex1, ey1);
    lv_obj_set_pos(s_snake.head_eye_r, ex2, ey2);
    lv_obj_set_pos(s_snake.head_tongue, tx, ty);
    lv_obj_set_size(s_snake.head_tongue, tw, th);
}

/*******************************************************************************
 * Direction Change (prevents 180-degree reversal)
 *******************************************************************************/
static void snake_set_direction(int8_t dx, int8_t dy)
{
    if (s_snake.snake_len > 1) {
        if (dx == -s_snake.dir_x && dy == -s_snake.dir_y) {
            return;
        }
    }
    s_snake.next_dir_x = dx;
    s_snake.next_dir_y = dy;
}

/*******************************************************************************
 * Game Over
 *******************************************************************************/
static void snake_game_over(const char *msg)
{
    s_snake.running = false;
    if (s_snake.score > s_snake_best) {
        s_snake_best = s_snake.score;
    }
    lv_label_set_text_fmt(s_snake.status_label,
        "%s  |  Tap " LV_SYMBOL_REFRESH " to Restart", msg);
    snake_update_hud();
}

/*******************************************************************************
 * Game Step (called every SNAKE_TICK_MS)
 *******************************************************************************/
static void snake_step(void)
{
    int16_t new_head_x, new_head_y;
    bool ate_food = false;
    int16_t i;

    s_snake.dir_x = s_snake.next_dir_x;
    s_snake.dir_y = s_snake.next_dir_y;

    new_head_x = (int16_t)(s_snake.snake_x[0] + s_snake.dir_x);
    new_head_y = (int16_t)(s_snake.snake_y[0] + s_snake.dir_y);

    /* Wall collision */
    if (new_head_x < 0 || new_head_x >= SNAKE_GRID_W ||
        new_head_y < 0 || new_head_y >= SNAKE_GRID_H) {
        snake_game_over("Wall hit");
        return;
    }

    /* Self collision */
    if (snake_contains_xy(new_head_x, new_head_y)) {
        snake_game_over("Self collision");
        return;
    }

    /* Food collision */
    if (new_head_x == s_snake.food_x && new_head_y == s_snake.food_y) {
        ate_food = true;
        if (s_snake.snake_len < SNAKE_MAX_LEN) {
            s_snake.snake_len++;
        }
        s_snake.score++;
        if (s_snake.score > s_snake_best) {
            s_snake_best = s_snake.score;
        }
    }

    /* Move body segments */
    for (i = (int16_t)s_snake.snake_len - 1; i > 0; i--) {
        s_snake.snake_x[i] = s_snake.snake_x[i - 1];
        s_snake.snake_y[i] = s_snake.snake_y[i - 1];
    }
    s_snake.snake_x[0] = new_head_x;
    s_snake.snake_y[0] = new_head_y;

    if (ate_food) {
        snake_spawn_food();
        lv_label_set_text(s_snake.status_label, "Nice bite! Keep going.");
    }

    snake_render();
    snake_update_hud();
}

/*******************************************************************************
 * Start / Restart
 *******************************************************************************/
static void snake_start(void)
{
    uint16_t i;

    s_snake.snake_len = 5;
    s_snake.score = 0;
    s_snake.running = true;

    s_snake.dir_x = 1;
    s_snake.dir_y = 0;
    s_snake.next_dir_x = 1;
    s_snake.next_dir_y = 0;

    for (i = 0; i < s_snake.snake_len; i++) {
        s_snake.snake_x[i] = (int16_t)(10 - i);
        s_snake.snake_y[i] = 10;
    }

    snake_spawn_food();
    snake_render();

    lv_label_set_text(s_snake.status_label,
        "D-pad: move. Eat food, avoid walls!");
    snake_update_hud();
}

/*******************************************************************************
 * Input Handling (touch D-pad - edge-triggered)
 *******************************************************************************/
static void snake_process_input(void)
{
    game_input_state_t input;
    game_input_read(&input);

    /* Restart */
    if ((input.restart && !s_snake.prev_input.restart) ||
        (!s_snake.running && input.action && !s_snake.prev_input.action)) {
        snake_start();
    }

    /* Direction change (edge-triggered to prevent rapid double-input) */
    if (s_snake.running) {
        if (input.up && !s_snake.prev_input.up) {
            snake_set_direction(0, -1);
        } else if (input.down && !s_snake.prev_input.down) {
            snake_set_direction(0, 1);
        } else if (input.left && !s_snake.prev_input.left) {
            snake_set_direction(-1, 0);
        } else if (input.right && !s_snake.prev_input.right) {
            snake_set_direction(1, 0);
        }
    }

    s_snake.prev_input = input;
}

/*******************************************************************************
 * Timer Callback (115ms - game logic tick)
 *******************************************************************************/
static void snake_tick_cb(lv_timer_t *timer)
{
    (void)timer;
    snake_process_input();
    if (s_snake.running) {
        snake_step();
    }
}

/*******************************************************************************
 * Example Entry Point
 *******************************************************************************/
void example_main(lv_obj_t *parent)
{
    lv_obj_t *arena;
    uint16_t i;

    /* Reset game state (keep best score) */
    uint32_t saved_best = s_snake_best;
    memset(&s_snake, 0, sizeof(s_snake));
    s_snake_best = saved_best;
    s_snake.parent = parent;

    /* Request USB HID joystick init (F310 support) */
    usb_hid_joystick_request_init();

    /* Dark background on parent */
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    /* Title label */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Snake");
    lv_obj_set_style_text_color(title, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    /* เกมงู */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "เกมงู");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 30);

    /* Score label */
    s_snake.score_label = lv_label_create(parent);
    lv_obj_set_style_text_color(s_snake.score_label, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_text_font(s_snake.score_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(s_snake.score_label, 16, 36);

    /* Game arena (centered) */
    arena = lv_obj_create(parent);
    lv_obj_set_size(arena, SNAKE_ARENA_W, SNAKE_ARENA_H);
    lv_obj_align(arena, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_pad_all(arena, 0, 0);
    lv_obj_set_style_radius(arena, 6, 0);
    lv_obj_set_style_border_width(arena, 2, 0);
    lv_obj_set_style_border_color(arena, gb_color(GB_DARKEST), 0);
    lv_obj_set_style_bg_color(arena, gb_color(GB_LIGHT), 0);
    lv_obj_set_style_shadow_width(arena, 0, 0);
    lv_obj_clear_flag(arena, LV_OBJ_FLAG_SCROLLABLE);
    s_snake.arena = arena;

    /* CRT scanlines */
    game_add_lcd_scanlines(arena, SNAKE_ARENA_W, SNAKE_ARENA_H);

    /* Snake segment objects (pre-allocated pool, hidden until needed) */
    for (i = 0; i < SNAKE_MAX_LEN; i++) {
        s_snake.segment_objs[i] = lv_obj_create(s_snake.arena);
        lv_obj_set_size(s_snake.segment_objs[i], SNAKE_CELL_PX, SNAKE_CELL_PX);
        lv_obj_set_style_radius(s_snake.segment_objs[i], 1, 0);
        lv_obj_set_style_border_width(s_snake.segment_objs[i], 1, 0);
        lv_obj_clear_flag(s_snake.segment_objs[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(s_snake.segment_objs[i], LV_OBJ_FLAG_HIDDEN);
    }

    /* Food object */
    s_snake.food_obj = lv_obj_create(s_snake.arena);
    lv_obj_set_size(s_snake.food_obj, SNAKE_CELL_PX, SNAKE_CELL_PX);
    lv_obj_set_style_bg_color(s_snake.food_obj, gb_color(GB_DARKEST), 0);
    lv_obj_set_style_border_color(s_snake.food_obj, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_border_width(s_snake.food_obj, 1, 0);
    lv_obj_set_style_radius(s_snake.food_obj, 1, 0);
    lv_obj_clear_flag(s_snake.food_obj, LV_OBJ_FLAG_SCROLLABLE);

    /* Snake eyes (small dots on head) */
    s_snake.head_eye_l = lv_obj_create(s_snake.arena);
    lv_obj_set_size(s_snake.head_eye_l, 3, 3);
    lv_obj_set_style_bg_color(s_snake.head_eye_l, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_border_width(s_snake.head_eye_l, 0, 0);
    lv_obj_set_style_radius(s_snake.head_eye_l, 1, 0);
    lv_obj_clear_flag(s_snake.head_eye_l, LV_OBJ_FLAG_SCROLLABLE);

    s_snake.head_eye_r = lv_obj_create(s_snake.arena);
    lv_obj_set_size(s_snake.head_eye_r, 3, 3);
    lv_obj_set_style_bg_color(s_snake.head_eye_r, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_border_width(s_snake.head_eye_r, 0, 0);
    lv_obj_set_style_radius(s_snake.head_eye_r, 1, 0);
    lv_obj_clear_flag(s_snake.head_eye_r, LV_OBJ_FLAG_SCROLLABLE);

    /* Tongue */
    s_snake.head_tongue = lv_obj_create(s_snake.arena);
    lv_obj_set_size(s_snake.head_tongue, 4, 2);
    lv_obj_set_style_bg_color(s_snake.head_tongue, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_border_width(s_snake.head_tongue, 0, 0);
    lv_obj_set_style_radius(s_snake.head_tongue, 0, 0);
    lv_obj_clear_flag(s_snake.head_tongue, LV_OBJ_FLAG_SCROLLABLE);

    /* Status label (overlaid on arena, top-left) */
    s_snake.status_label = lv_label_create(arena);
    lv_obj_set_width(s_snake.status_label, SNAKE_ARENA_W - 100);
    lv_obj_set_style_text_color(s_snake.status_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(s_snake.status_label, LV_OPA_70, 0);
    lv_obj_set_style_text_font(s_snake.status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(s_snake.status_label, 8, 6);
    lv_obj_add_flag(s_snake.status_label, LV_OBJ_FLAG_IGNORE_LAYOUT);

    /* Touch overlay controls: full D-pad + Restart */
    lv_obj_update_layout(parent);
    lv_coord_t ax = lv_obj_get_x(arena);
    lv_coord_t ay = lv_obj_get_y(arena);
    game_create_touch_dpad(parent, ax, ay, SNAKE_ARENA_H, true);
    game_create_touch_restart(parent);

    /* Start game timer */
    s_snake.timer = lv_timer_create(snake_tick_cb, SNAKE_TICK_MS, NULL);

    /* Start first round */
    snake_start();
}
