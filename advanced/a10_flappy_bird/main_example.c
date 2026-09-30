/*******************************************************************************
 * A09 - Flappy Bird (Touch Control)
 *
 * Production-derived Flappy Bird game for Developer Hub.
 * Adapted from page_game_flappy.c (TESAIoT Game Console).
 *
 * 480x384 arena, gravity 0.42, flap velocity -6.8, 3 pipes.
 * 20ms tick timer for smooth 50fps physics.
 * Touch: tap Action button or tap arena to flap.
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
#define FLAP_ARENA_W         480
#define FLAP_ARENA_H         384
#define FLAP_PIPE_COUNT      3
#define FLAP_PIPE_W          76
#define FLAP_PIPE_GAP        130
#define FLAP_PIPE_SPACING    210
#define FLAP_BIRD_X          120
#define FLAP_BIRD_SIZE       24
#define FLAP_TICK_MS         20

/*******************************************************************************
 * Pipe Structure
 *******************************************************************************/
typedef struct {
    lv_obj_t *top;
    lv_obj_t *bottom;
    int32_t x;
    int32_t gap_y;
    bool passed;
} flap_pipe_t;

/*******************************************************************************
 * Game State
 *******************************************************************************/
typedef struct {
    lv_obj_t *parent;
    lv_obj_t *arena;
    lv_obj_t *score_label;
    lv_obj_t *status_label;
    lv_obj_t *bird;
    lv_obj_t *bird_wing;
    lv_obj_t *bird_eye;
    lv_obj_t *bird_beak;
    lv_timer_t *timer;

    flap_pipe_t pipes[FLAP_PIPE_COUNT];
    float bird_y;
    float bird_vy;
    uint8_t wing_phase;
    uint32_t score;
    bool running;
    game_input_state_t prev_input;
} flappy_state_t;

static flappy_state_t s_flap;
static uint32_t s_flap_best;

/*******************************************************************************
 * Forward Declarations
 *******************************************************************************/
static void flappy_start(void);
static void flappy_step(void);

/*******************************************************************************
 * Arena tap callback - tap anywhere on arena to flap
 *******************************************************************************/
static void arena_tap_cb(lv_event_t *e)
{
    (void)e;
    if (!s_flap.running) {
        flappy_start();
    } else {
        s_flap.bird_vy = -6.8f;
        lv_label_set_text(s_flap.status_label, "Flap!");
    }
}

/*******************************************************************************
 * HUD
 *******************************************************************************/
static void flappy_update_hud(void)
{
    if (s_flap.score_label == NULL) return;
    lv_label_set_text_fmt(s_flap.score_label,
        "Score: %lu    Best: %lu",
        (unsigned long)s_flap.score,
        (unsigned long)s_flap_best);
}

/*******************************************************************************
 * Pipe Management
 *******************************************************************************/
static int32_t flappy_next_pipe_x(void)
{
    int32_t max_x = 0;
    uint32_t i;
    for (i = 0; i < FLAP_PIPE_COUNT; i++) {
        if (s_flap.pipes[i].x > max_x) max_x = s_flap.pipes[i].x;
    }
    return max_x;
}

static void flappy_reset_pipe(uint32_t i, bool initial)
{
    int32_t x = initial
        ? FLAP_ARENA_W + (int32_t)i * FLAP_PIPE_SPACING
        : flappy_next_pipe_x() + FLAP_PIPE_SPACING;

    s_flap.pipes[i].x = x;
    s_flap.pipes[i].gap_y = (int32_t)lv_rand(95, FLAP_ARENA_H - 95);
    s_flap.pipes[i].passed = false;
}

static void flappy_render_pipe(uint32_t i)
{
    int32_t gap_top = s_flap.pipes[i].gap_y - FLAP_PIPE_GAP / 2;
    int32_t gap_bottom = s_flap.pipes[i].gap_y + FLAP_PIPE_GAP / 2;
    if (gap_top < 20) gap_top = 20;
    if (gap_bottom > FLAP_ARENA_H - 20) gap_bottom = FLAP_ARENA_H - 20;

    lv_obj_set_size(s_flap.pipes[i].top, FLAP_PIPE_W, gap_top);
    lv_obj_set_pos(s_flap.pipes[i].top, s_flap.pipes[i].x, 0);
    lv_obj_set_size(s_flap.pipes[i].bottom, FLAP_PIPE_W, FLAP_ARENA_H - gap_bottom);
    lv_obj_set_pos(s_flap.pipes[i].bottom, s_flap.pipes[i].x, gap_bottom);
}

static bool flappy_hit_pipe(uint32_t i, int32_t bird_x, int32_t bird_y)
{
    int32_t bird_r = FLAP_BIRD_SIZE / 2;
    int32_t left = bird_x - bird_r;
    int32_t right = bird_x + bird_r;
    int32_t top = bird_y - bird_r;
    int32_t bottom = bird_y + bird_r;

    int32_t pipe_left = s_flap.pipes[i].x;
    int32_t pipe_right = s_flap.pipes[i].x + FLAP_PIPE_W;
    int32_t gap_top = s_flap.pipes[i].gap_y - FLAP_PIPE_GAP / 2;
    int32_t gap_bottom = s_flap.pipes[i].gap_y + FLAP_PIPE_GAP / 2;

    bool x_overlap = right >= pipe_left && left <= pipe_right;
    if (!x_overlap) return false;
    if (top < gap_top || bottom > gap_bottom) return true;
    return false;
}

/*******************************************************************************
 * Game Over / Flap / Step
 *******************************************************************************/
static void flappy_game_over(const char *msg)
{
    s_flap.running = false;
    if (s_flap.score > s_flap_best) s_flap_best = s_flap.score;
    lv_label_set_text_fmt(s_flap.status_label,
        "%s  |  Tap " LV_SYMBOL_REFRESH " or Arena to Restart", msg);
    flappy_update_hud();
}

static void flappy_flap(void)
{
    if (!s_flap.running) return;
    s_flap.bird_vy = -6.8f;
    lv_label_set_text(s_flap.status_label, "Flap!");
}

static void flappy_step(void)
{
    uint32_t i;
    int32_t bird_draw_y;

    s_flap.bird_vy += 0.42f;
    s_flap.bird_y += s_flap.bird_vy;

    if (s_flap.bird_y < (float)(FLAP_BIRD_SIZE / 2)) {
        s_flap.bird_y = (float)(FLAP_BIRD_SIZE / 2);
        s_flap.bird_vy = 0.0f;
    }

    if (s_flap.bird_y > (float)(FLAP_ARENA_H - FLAP_BIRD_SIZE / 2)) {
        flappy_game_over("Ground hit");
        return;
    }

    bird_draw_y = (int32_t)(s_flap.bird_y - (float)(FLAP_BIRD_SIZE / 2));
    lv_obj_set_pos(s_flap.bird, FLAP_BIRD_X - FLAP_BIRD_SIZE / 2, bird_draw_y);

    /* Wing animation */
    s_flap.wing_phase++;
    if ((s_flap.wing_phase & 1U) == 0U) {
        lv_obj_set_pos(s_flap.bird_wing, 3, 10);
        lv_obj_set_size(s_flap.bird_wing, 10, 7);
    } else {
        lv_obj_set_pos(s_flap.bird_wing, 2, 7);
        lv_obj_set_size(s_flap.bird_wing, 12, 8);
    }

    /* Move pipes and check collision */
    for (i = 0; i < FLAP_PIPE_COUNT; i++) {
        s_flap.pipes[i].x -= 3;

        if (!s_flap.pipes[i].passed &&
            s_flap.pipes[i].x + FLAP_PIPE_W < FLAP_BIRD_X) {
            s_flap.pipes[i].passed = true;
            s_flap.score++;
            if (s_flap.score > s_flap_best) s_flap_best = s_flap.score;
            flappy_update_hud();
        }

        if (s_flap.pipes[i].x + FLAP_PIPE_W < 0) {
            flappy_reset_pipe(i, false);
        }

        flappy_render_pipe(i);

        if (flappy_hit_pipe(i, FLAP_BIRD_X, (int32_t)s_flap.bird_y)) {
            flappy_game_over("Pipe collision");
            return;
        }
    }
}

/*******************************************************************************
 * Start / Restart
 *******************************************************************************/
static void flappy_start(void)
{
    uint32_t i;
    s_flap.score = 0;
    s_flap.bird_y = (float)(FLAP_ARENA_H / 2);
    s_flap.bird_vy = 0.0f;
    s_flap.wing_phase = 0;
    s_flap.running = true;

    for (i = 0; i < FLAP_PIPE_COUNT; i++) {
        flappy_reset_pipe(i, true);
        flappy_render_pipe(i);
    }

    lv_obj_set_pos(s_flap.bird,
                   FLAP_BIRD_X - FLAP_BIRD_SIZE / 2,
                   (int32_t)s_flap.bird_y - FLAP_BIRD_SIZE / 2);
    lv_obj_set_pos(s_flap.bird_wing, 3, 10);
    lv_obj_set_size(s_flap.bird_wing, 10, 7);

    lv_label_set_text(s_flap.status_label, "Tap A or Arena to flap. Avoid pipes!");
    flappy_update_hud();
}

/*******************************************************************************
 * Input Handling (touch-only)
 *******************************************************************************/
static void flappy_process_input(void)
{
    game_input_state_t input;
    bool flap_edge;

    game_input_read(&input);

    /* Flap on: A/B/X button OR D-pad Up OR stick Up (edge-triggered) */
    flap_edge = (input.action && !s_flap.prev_input.action) ||
                (input.up && !s_flap.prev_input.up);

    /* Restart on: Y button (edge-triggered), or flap when not running */
    if ((input.restart && !s_flap.prev_input.restart) ||
        (!s_flap.running && flap_edge)) {
        flappy_start();
    } else if (flap_edge) {
        flappy_flap();
    }

    s_flap.prev_input = input;
}

/*******************************************************************************
 * Timer Callback
 *******************************************************************************/
static void flappy_tick_cb(lv_timer_t *timer)
{
    (void)timer;
    flappy_process_input();
    if (s_flap.running) flappy_step();
}

/*******************************************************************************
 * Example Entry Point
 *******************************************************************************/
void example_main(lv_obj_t *parent)
{
    lv_obj_t *arena;
    uint32_t i;

    uint32_t saved_best = s_flap_best;
    memset(&s_flap, 0, sizeof(s_flap));
    s_flap_best = saved_best;
    s_flap.parent = parent;

    /* Request USB HID joystick init (F310 support) */
    usb_hid_joystick_request_init();

    /* Dark background on parent */
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    /* Title label */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Flappy Bird");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFC107), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    /* เกม Flappy Bird */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "เกม Flappy Bird");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 30);

    /* Score label */
    s_flap.score_label = lv_label_create(parent);
    lv_obj_set_style_text_color(s_flap.score_label, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_text_font(s_flap.score_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(s_flap.score_label, 16, 36);

    /* Arena (centered) */
    arena = lv_obj_create(parent);
    lv_obj_set_size(arena, FLAP_ARENA_W, FLAP_ARENA_H);
    lv_obj_align(arena, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_pad_all(arena, 0, 0);
    lv_obj_set_style_radius(arena, 6, 0);
    lv_obj_set_style_border_width(arena, 2, 0);
    lv_obj_set_style_border_color(arena, gb_color(GB_DARKEST), 0);
    lv_obj_set_style_bg_color(arena, gb_color(GB_LIGHT), 0);
    lv_obj_set_style_shadow_width(arena, 0, 0);
    lv_obj_clear_flag(arena, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(arena, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(arena, arena_tap_cb, LV_EVENT_CLICKED, NULL);
    s_flap.arena = arena;

    /* CRT scanlines */
    game_add_lcd_scanlines(arena, FLAP_ARENA_W, FLAP_ARENA_H);

    /* Pipes (pre-allocated) */
    for (i = 0; i < FLAP_PIPE_COUNT; i++) {
        s_flap.pipes[i].top = lv_obj_create(arena);
        s_flap.pipes[i].bottom = lv_obj_create(arena);

        lv_obj_set_style_bg_color(s_flap.pipes[i].top, gb_color(GB_DARK), 0);
        lv_obj_set_style_border_color(s_flap.pipes[i].top, gb_color(GB_DARKEST), 0);
        lv_obj_set_style_border_width(s_flap.pipes[i].top, 2, 0);
        lv_obj_set_style_radius(s_flap.pipes[i].top, 0, 0);
        lv_obj_clear_flag(s_flap.pipes[i].top, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_set_style_bg_color(s_flap.pipes[i].bottom, gb_color(GB_DARK), 0);
        lv_obj_set_style_border_color(s_flap.pipes[i].bottom, gb_color(GB_DARKEST), 0);
        lv_obj_set_style_border_width(s_flap.pipes[i].bottom, 2, 0);
        lv_obj_set_style_radius(s_flap.pipes[i].bottom, 0, 0);
        lv_obj_clear_flag(s_flap.pipes[i].bottom, LV_OBJ_FLAG_SCROLLABLE);
    }

    /* Bird body */
    s_flap.bird = lv_obj_create(arena);
    lv_obj_set_size(s_flap.bird, FLAP_BIRD_SIZE, FLAP_BIRD_SIZE);
    lv_obj_set_style_radius(s_flap.bird, FLAP_BIRD_SIZE / 2, 0);
    lv_obj_set_style_bg_color(s_flap.bird, gb_color(GB_DARKEST), 0);
    lv_obj_set_style_border_color(s_flap.bird, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_border_width(s_flap.bird, 2, 0);
    lv_obj_clear_flag(s_flap.bird, LV_OBJ_FLAG_SCROLLABLE);

    /* Wing */
    s_flap.bird_wing = lv_obj_create(s_flap.bird);
    lv_obj_set_size(s_flap.bird_wing, 10, 7);
    lv_obj_set_pos(s_flap.bird_wing, 3, 10);
    lv_obj_set_style_bg_color(s_flap.bird_wing, gb_color(GB_DARK), 0);
    lv_obj_set_style_border_color(s_flap.bird_wing, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_border_width(s_flap.bird_wing, 1, 0);
    lv_obj_set_style_radius(s_flap.bird_wing, 3, 0);
    lv_obj_clear_flag(s_flap.bird_wing, LV_OBJ_FLAG_SCROLLABLE);

    /* Eye */
    s_flap.bird_eye = lv_obj_create(s_flap.bird);
    lv_obj_set_size(s_flap.bird_eye, 4, 4);
    lv_obj_set_pos(s_flap.bird_eye, FLAP_BIRD_SIZE - 8, 6);
    lv_obj_set_style_bg_color(s_flap.bird_eye, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_border_width(s_flap.bird_eye, 0, 0);
    lv_obj_set_style_radius(s_flap.bird_eye, 2, 0);
    lv_obj_clear_flag(s_flap.bird_eye, LV_OBJ_FLAG_SCROLLABLE);

    /* Beak */
    s_flap.bird_beak = lv_obj_create(s_flap.bird);
    lv_obj_set_size(s_flap.bird_beak, 6, 4);
    lv_obj_set_pos(s_flap.bird_beak, FLAP_BIRD_SIZE - 1, 10);
    lv_obj_set_style_bg_color(s_flap.bird_beak, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_border_color(s_flap.bird_beak, gb_color(GB_DARKEST), 0);
    lv_obj_set_style_border_width(s_flap.bird_beak, 1, 0);
    lv_obj_set_style_radius(s_flap.bird_beak, 1, 0);
    lv_obj_clear_flag(s_flap.bird_beak, LV_OBJ_FLAG_SCROLLABLE);

    /* Status label (overlaid on arena) */
    s_flap.status_label = lv_label_create(arena);
    lv_obj_set_width(s_flap.status_label, FLAP_ARENA_W - 100);
    lv_obj_set_style_text_color(s_flap.status_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(s_flap.status_label, LV_OPA_70, 0);
    lv_obj_set_style_text_font(s_flap.status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(s_flap.status_label, 8, 6);
    lv_obj_add_flag(s_flap.status_label, LV_OBJ_FLAG_IGNORE_LAYOUT);

    /* Touch overlay controls */
    lv_obj_update_layout(parent);
    lv_coord_t ax = lv_obj_get_x(arena);
    lv_coord_t ay = lv_obj_get_y(arena);
    game_create_touch_action(parent, ax, FLAP_ARENA_W, ay, FLAP_ARENA_H);
    game_create_touch_restart(parent);

    /* Timer + Start */
    s_flap.timer = lv_timer_create(flappy_tick_cb, FLAP_TICK_MS, NULL);
    flappy_start();
}
