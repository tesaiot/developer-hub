/*******************************************************************************
 * A12 - Space Shooter (Touch Control)
 *
 * Production-derived Space Shooter game for Developer Hub.
 * Adapted from page_game_shooter.c (TESAIoT Game Console, 467 lines).
 *
 * 480x384 arena, ship at bottom (44x20), 6 bullet pool, 8 enemy pool.
 * Ship has wings + cabin sprites (production-accurate).
 * Enemies have dual eyes (production-accurate).
 * 20ms tick timer, ship moves L/R at 7px, bullets at 7px up,
 * enemies 1.8-3.4px down. Spawn cooldown, 3 lives, progressive difficulty.
 * Entity pool pattern: pre-allocated bullets[6] + enemies[8], active flags.
 * AABB collision: bullet-enemy, ship-enemy, enemy-past-bottom.
 * Touch: D-pad L/R + Action (fire) + Restart.
 *
 * Entry point: void example_main(lv_obj_t *parent)
 *
 *******************************************************************************/

#include "app_interface.h"
#include "tesaiot_thai.h"
#include "game_common.h"
#include "usb_hid_joystick.h"

/*******************************************************************************
 * Game Constants
 *******************************************************************************/
#define SHOOT_ARENA_W          480
#define SHOOT_ARENA_H          384
#define SHOOT_SHIP_W           44
#define SHOOT_SHIP_H           20
#define SHOOT_SHIP_Y           (SHOOT_ARENA_H - SHOOT_SHIP_H - 16)
#define SHOOT_BULLET_MAX       6
#define SHOOT_ENEMY_MAX        8
#define SHOOT_TICK_MS          20

/*******************************************************************************
 * Entity Structures
 *******************************************************************************/
typedef struct {
    lv_obj_t *obj;
    bool active;
    float x;
    float y;
} shooter_bullet_t;

typedef struct {
    lv_obj_t *obj;
    bool active;
    float x;
    float y;
    float vy;
} shooter_enemy_t;

/*******************************************************************************
 * Game State
 *******************************************************************************/
typedef struct {
    lv_obj_t *parent;
    lv_obj_t *arena;
    lv_obj_t *score_label;
    lv_obj_t *status_label;
    lv_obj_t *ship;
    lv_timer_t *timer;

    shooter_bullet_t bullets[SHOOT_BULLET_MAX];
    shooter_enemy_t enemies[SHOOT_ENEMY_MAX];
    float ship_x;
    uint32_t score;
    int32_t lives;
    uint16_t spawn_cd;
    bool running;
    game_input_state_t prev_input;
} shooter_state_t;

static shooter_state_t s_shooter;
static uint32_t s_shooter_best;

/*******************************************************************************
 * Forward Declarations
 *******************************************************************************/
static void shooter_start(void);

/*******************************************************************************
 * Helpers
 *******************************************************************************/
static bool shooter_overlap(float ax, float ay, float aw, float ah,
                             float bx, float by, float bw, float bh)
{
    if (ax + aw < bx) return false;
    if (ax > bx + bw) return false;
    if (ay + ah < by) return false;
    if (ay > by + bh) return false;
    return true;
}

static void shooter_update_hud(void)
{
    if (s_shooter.score_label == NULL) return;
    lv_label_set_text_fmt(s_shooter.score_label,
        "Score: %lu    Best: %lu    Lives: %ld",
        (unsigned long)s_shooter.score,
        (unsigned long)s_shooter_best,
        (long)s_shooter.lives);
}

static void shooter_set_ship_x(float x)
{
    if (x < 0.0f) x = 0.0f;
    if (x > (float)(SHOOT_ARENA_W - SHOOT_SHIP_W))
        x = (float)(SHOOT_ARENA_W - SHOOT_SHIP_W);
    s_shooter.ship_x = x;
    lv_obj_set_pos(s_shooter.ship, (int32_t)s_shooter.ship_x, SHOOT_SHIP_Y);
}

static void shooter_reset_bullet(uint32_t i)
{
    s_shooter.bullets[i].active = false;
    s_shooter.bullets[i].x = -20.0f;
    s_shooter.bullets[i].y = -20.0f;
    lv_obj_add_flag(s_shooter.bullets[i].obj, LV_OBJ_FLAG_HIDDEN);
}

static void shooter_reset_enemy(uint32_t i)
{
    s_shooter.enemies[i].active = false;
    s_shooter.enemies[i].x = -30.0f;
    s_shooter.enemies[i].y = -30.0f;
    lv_obj_add_flag(s_shooter.enemies[i].obj, LV_OBJ_FLAG_HIDDEN);
}

static void shooter_spawn_enemy(void)
{
    uint32_t i;
    for (i = 0; i < SHOOT_ENEMY_MAX; i++) {
        if (!s_shooter.enemies[i].active) {
            s_shooter.enemies[i].active = true;
            s_shooter.enemies[i].x = (float)lv_rand(20, SHOOT_ARENA_W - 46);
            s_shooter.enemies[i].y = -20.0f;
            /* Random speed 1.8 - 3.4 px/tick */
            s_shooter.enemies[i].vy = ((float)lv_rand(18, 34)) / 10.0f;
            lv_obj_clear_flag(s_shooter.enemies[i].obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(s_shooter.enemies[i].obj,
                           (int32_t)s_shooter.enemies[i].x,
                           (int32_t)s_shooter.enemies[i].y);
            return;
        }
    }
}

static void shooter_fire(void)
{
    uint32_t i;
    if (!s_shooter.running) return;
    for (i = 0; i < SHOOT_BULLET_MAX; i++) {
        if (!s_shooter.bullets[i].active) {
            s_shooter.bullets[i].active = true;
            s_shooter.bullets[i].x = s_shooter.ship_x +
                                      (SHOOT_SHIP_W / 2.0f) - 2.0f;
            s_shooter.bullets[i].y = (float)SHOOT_SHIP_Y - 8.0f;
            lv_obj_clear_flag(s_shooter.bullets[i].obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(s_shooter.bullets[i].obj,
                           (int32_t)s_shooter.bullets[i].x,
                           (int32_t)s_shooter.bullets[i].y);
            return;
        }
    }
}

/*******************************************************************************
 * Game Over / Start
 *******************************************************************************/
static void shooter_game_over(const char *msg)
{
    s_shooter.running = false;
    if (s_shooter.score > s_shooter_best) s_shooter_best = s_shooter.score;
    lv_label_set_text_fmt(s_shooter.status_label,
        "%s  |  Tap " LV_SYMBOL_REFRESH " to Restart", msg);
    shooter_update_hud();
}

static void shooter_start(void)
{
    uint32_t i;
    s_shooter.score = 0;
    s_shooter.lives = 3;
    s_shooter.spawn_cd = 8;
    s_shooter.running = true;

    shooter_set_ship_x((float)(SHOOT_ARENA_W / 2 - SHOOT_SHIP_W / 2));

    for (i = 0; i < SHOOT_BULLET_MAX; i++) shooter_reset_bullet(i);
    for (i = 0; i < SHOOT_ENEMY_MAX; i++) shooter_reset_enemy(i);

    /* Initial wave */
    shooter_spawn_enemy();
    shooter_spawn_enemy();
    shooter_spawn_enemy();

    lv_label_set_text(s_shooter.status_label, "LEFT/RIGHT + A to fire!");
    shooter_update_hud();
}

/*******************************************************************************
 * Game Step
 *******************************************************************************/
static void shooter_step(void)
{
    uint32_t i, j;

    /* Spawn cooldown timer */
    if (s_shooter.spawn_cd > 0U) {
        s_shooter.spawn_cd--;
    } else {
        shooter_spawn_enemy();
        s_shooter.spawn_cd = 18U;
    }

    /* Move bullets upward at 7px/tick */
    for (i = 0; i < SHOOT_BULLET_MAX; i++) {
        if (!s_shooter.bullets[i].active) continue;
        s_shooter.bullets[i].y -= 7.0f;
        if (s_shooter.bullets[i].y < -12.0f) {
            shooter_reset_bullet(i);
            continue;
        }
        lv_obj_set_pos(s_shooter.bullets[i].obj,
                       (int32_t)s_shooter.bullets[i].x,
                       (int32_t)s_shooter.bullets[i].y);
    }

    /* Move enemies downward + collision detection */
    for (i = 0; i < SHOOT_ENEMY_MAX; i++) {
        bool removed = false;
        if (!s_shooter.enemies[i].active) continue;

        s_shooter.enemies[i].y += s_shooter.enemies[i].vy;
        lv_obj_set_pos(s_shooter.enemies[i].obj,
                       (int32_t)s_shooter.enemies[i].x,
                       (int32_t)s_shooter.enemies[i].y);

        /* Enemy passed bottom - lose a life */
        if (s_shooter.enemies[i].y > (float)SHOOT_ARENA_H) {
            shooter_reset_enemy(i);
            s_shooter.lives--;
            if (s_shooter.lives <= 0) {
                shooter_game_over("Base destroyed");
                return;
            }
            continue;
        }

        /* AABB: Bullet-enemy collision */
        for (j = 0; j < SHOOT_BULLET_MAX; j++) {
            if (!s_shooter.bullets[j].active) continue;
            if (shooter_overlap(
                    s_shooter.bullets[j].x, s_shooter.bullets[j].y,
                    4.0f, 10.0f,
                    s_shooter.enemies[i].x, s_shooter.enemies[i].y,
                    26.0f, 16.0f)) {
                shooter_reset_bullet(j);
                shooter_reset_enemy(i);
                removed = true;
                s_shooter.score++;
                if (s_shooter.score > s_shooter_best)
                    s_shooter_best = s_shooter.score;
                shooter_spawn_enemy();
                break;
            }
        }
        if (removed) continue;

        /* AABB: Ship-enemy collision */
        if (shooter_overlap(
                s_shooter.ship_x, (float)SHOOT_SHIP_Y,
                (float)SHOOT_SHIP_W, (float)SHOOT_SHIP_H,
                s_shooter.enemies[i].x, s_shooter.enemies[i].y,
                26.0f, 16.0f)) {
            shooter_reset_enemy(i);
            s_shooter.lives--;
            if (s_shooter.lives <= 0) {
                shooter_game_over("Ship collision");
                return;
            }
        }
    }

    shooter_update_hud();
}

/*******************************************************************************
 * Input Handling (touch-only, no F310, no page_manager)
 *******************************************************************************/
static void shooter_process_input(void)
{
    game_input_state_t input;
    bool action_edge;

    game_input_read(&input);

    action_edge = input.action && !s_shooter.prev_input.action;

    if ((input.restart && !s_shooter.prev_input.restart) ||
        (!s_shooter.running && action_edge)) {
        shooter_start();
    }

    if (s_shooter.running) {
        /* Continuous hold for smooth ship movement at 7px/tick */
        if (input.left) shooter_set_ship_x(s_shooter.ship_x - 7.0f);
        if (input.right) shooter_set_ship_x(s_shooter.ship_x + 7.0f);
        if (action_edge) shooter_fire();
    }

    s_shooter.prev_input = input;
}

/*******************************************************************************
 * Timer Callback (20ms = 50fps game loop)
 *******************************************************************************/
static void shooter_tick_cb(lv_timer_t *timer)
{
    (void)timer;
    shooter_process_input();
    if (s_shooter.running) shooter_step();
}

/*******************************************************************************
 * Example Entry Point
 *******************************************************************************/
void example_main(lv_obj_t *parent)
{
    tesaiot_add_thai_support_badge();

    lv_obj_t *arena;
    uint32_t i;

    uint32_t saved_best = s_shooter_best;
    memset(&s_shooter, 0, sizeof(s_shooter));
    s_shooter_best = saved_best;
    s_shooter.parent = parent;

    /* Request USB HID joystick init (F310 support) */
    usb_hid_joystick_request_init();

    /* Dark background on parent */
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Space Shooter");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF44336), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    /* เกมยิง */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "เกมยิง");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 30);

    /* Score + Lives HUD */
    s_shooter.score_label = lv_label_create(parent);
    lv_obj_set_style_text_color(s_shooter.score_label,
                                 gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_text_font(s_shooter.score_label,
                                &lv_font_montserrat_14, 0);
    lv_obj_set_pos(s_shooter.score_label, 16, 36);

    /* Arena - Game Boy palette background */
    arena = lv_obj_create(parent);
    lv_obj_set_size(arena, SHOOT_ARENA_W, SHOOT_ARENA_H);
    lv_obj_align(arena, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_pad_all(arena, 0, 0);
    lv_obj_set_style_radius(arena, 6, 0);
    lv_obj_set_style_border_width(arena, 2, 0);
    lv_obj_set_style_border_color(arena, gb_color(GB_DARKEST), 0);
    lv_obj_set_style_bg_color(arena, gb_color(GB_LIGHT), 0);
    lv_obj_set_style_shadow_width(arena, 0, 0);
    lv_obj_clear_flag(arena, LV_OBJ_FLAG_SCROLLABLE);
    s_shooter.arena = arena;

    /* CRT scanlines */
    game_add_lcd_scanlines(arena, SHOOT_ARENA_W, SHOOT_ARENA_H);

    /* Ship body (44x20 px) */
    s_shooter.ship = lv_obj_create(arena);
    lv_obj_set_size(s_shooter.ship, SHOOT_SHIP_W, SHOOT_SHIP_H);
    lv_obj_set_style_bg_color(s_shooter.ship, gb_color(GB_DARKEST), 0);
    lv_obj_set_style_border_color(s_shooter.ship, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_border_width(s_shooter.ship, 1, 0);
    lv_obj_set_style_radius(s_shooter.ship, 2, 0);
    lv_obj_clear_flag(s_shooter.ship, LV_OBJ_FLAG_SCROLLABLE);
    {
        /* Left wing */
        lv_obj_t *wing_l = lv_obj_create(s_shooter.ship);
        lv_obj_set_size(wing_l, 8, 8);
        lv_obj_set_pos(wing_l, 0, 10);
        lv_obj_set_style_bg_color(wing_l, gb_color(GB_DARK), 0);
        lv_obj_set_style_border_width(wing_l, 0, 0);
        lv_obj_set_style_radius(wing_l, 1, 0);
        lv_obj_clear_flag(wing_l, LV_OBJ_FLAG_SCROLLABLE);

        /* Right wing */
        lv_obj_t *wing_r = lv_obj_create(s_shooter.ship);
        lv_obj_set_size(wing_r, 8, 8);
        lv_obj_set_pos(wing_r, SHOOT_SHIP_W - 8, 10);
        lv_obj_set_style_bg_color(wing_r, gb_color(GB_DARK), 0);
        lv_obj_set_style_border_width(wing_r, 0, 0);
        lv_obj_set_style_radius(wing_r, 1, 0);
        lv_obj_clear_flag(wing_r, LV_OBJ_FLAG_SCROLLABLE);

        /* Cabin */
        lv_obj_t *cabin = lv_obj_create(s_shooter.ship);
        lv_obj_set_size(cabin, 12, 8);
        lv_obj_set_pos(cabin, 16, 0);
        lv_obj_set_style_bg_color(cabin, gb_color(GB_LIGHTEST), 0);
        lv_obj_set_style_border_color(cabin, gb_color(GB_DARKEST), 0);
        lv_obj_set_style_border_width(cabin, 1, 0);
        lv_obj_set_style_radius(cabin, 1, 0);
        lv_obj_clear_flag(cabin, LV_OBJ_FLAG_SCROLLABLE);
    }

    /* Bullet pool (pre-allocated, hidden until fired) */
    for (i = 0; i < SHOOT_BULLET_MAX; i++) {
        s_shooter.bullets[i].obj = lv_obj_create(arena);
        lv_obj_set_size(s_shooter.bullets[i].obj, 4, 10);
        lv_obj_set_style_bg_color(s_shooter.bullets[i].obj,
                                   gb_color(GB_DARKEST), 0);
        lv_obj_set_style_border_width(s_shooter.bullets[i].obj, 0, 0);
        lv_obj_set_style_radius(s_shooter.bullets[i].obj, 1, 0);
        lv_obj_clear_flag(s_shooter.bullets[i].obj, LV_OBJ_FLAG_SCROLLABLE);
        shooter_reset_bullet(i);
    }

    /* Enemy pool (pre-allocated, dual eyes, hidden until spawned) */
    for (i = 0; i < SHOOT_ENEMY_MAX; i++) {
        s_shooter.enemies[i].obj = lv_obj_create(arena);
        lv_obj_set_size(s_shooter.enemies[i].obj, 26, 16);
        lv_obj_set_style_bg_color(s_shooter.enemies[i].obj,
                                   gb_color(GB_DARK), 0);
        lv_obj_set_style_border_color(s_shooter.enemies[i].obj,
                                       gb_color(GB_DARKEST), 0);
        lv_obj_set_style_border_width(s_shooter.enemies[i].obj, 1, 0);
        lv_obj_set_style_radius(s_shooter.enemies[i].obj, 2, 0);
        lv_obj_clear_flag(s_shooter.enemies[i].obj, LV_OBJ_FLAG_SCROLLABLE);
        {
            /* Left eye */
            lv_obj_t *eye1 = lv_obj_create(s_shooter.enemies[i].obj);
            lv_obj_set_size(eye1, 4, 4);
            lv_obj_set_pos(eye1, 5, 4);
            lv_obj_set_style_bg_color(eye1, gb_color(GB_LIGHTEST), 0);
            lv_obj_set_style_border_width(eye1, 0, 0);
            lv_obj_set_style_radius(eye1, 1, 0);
            lv_obj_clear_flag(eye1, LV_OBJ_FLAG_SCROLLABLE);

            /* Right eye */
            lv_obj_t *eye2 = lv_obj_create(s_shooter.enemies[i].obj);
            lv_obj_set_size(eye2, 4, 4);
            lv_obj_set_pos(eye2, 17, 4);
            lv_obj_set_style_bg_color(eye2, gb_color(GB_LIGHTEST), 0);
            lv_obj_set_style_border_width(eye2, 0, 0);
            lv_obj_set_style_radius(eye2, 1, 0);
            lv_obj_clear_flag(eye2, LV_OBJ_FLAG_SCROLLABLE);
        }
        shooter_reset_enemy(i);
    }

    /* Status label overlaid on arena bottom */
    s_shooter.status_label = lv_label_create(arena);
    lv_obj_set_width(s_shooter.status_label, SHOOT_ARENA_W - 20);
    lv_obj_set_style_text_color(s_shooter.status_label,
                                 gb_color(GB_DARKEST), 0);
    lv_obj_set_style_text_font(s_shooter.status_label,
                                &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(s_shooter.status_label,
                                 LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_shooter.status_label, LV_ALIGN_BOTTOM_MID, 0, -4);

    /* Touch overlay controls */
    lv_obj_update_layout(parent);
    lv_coord_t ax = lv_obj_get_x(arena);
    lv_coord_t ay = lv_obj_get_y(arena);
    game_create_touch_dpad(parent, ax, ay, SHOOT_ARENA_H, true);
    game_create_touch_action(parent, ax, SHOOT_ARENA_W, ay, SHOOT_ARENA_H);
    game_create_touch_restart(parent);

    /* Timer + Start */
    s_shooter.timer = lv_timer_create(shooter_tick_cb, SHOOT_TICK_MS, NULL);
    shooter_start();
}
