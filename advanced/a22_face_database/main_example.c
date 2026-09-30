/**
 * @file    main_example.c
 * @brief   Face Database Manager - Enrollment list, add/delete, status display
 *
 * @description
 *   Face database management UI concept demo. Shows a scrollable user list
 *   with enrollment status indicators, add/delete controls, simulated
 *   enrollment progress bar, capacity gauge, and per-entry detail panel.
 *   Pure LVGL demo - no camera or face detection hardware required.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "pse84_common.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define MAX_USERS           8
#define NAME_MAX_LEN        16
#define ENROLL_STEPS        20
#define ENROLL_INTERVAL_MS  80

/* ---------------------------------------------------------------------------
 * User entry
 * --------------------------------------------------------------------------- */
typedef enum {
    STATUS_EMPTY = 0,
    STATUS_ENROLLED,
    STATUS_ENROLLING,
} user_status_t;

typedef struct {
    char            name[NAME_MAX_LEN];
    user_status_t   status;
    int             confidence;     /* 0..100 simulated match quality */
    uint32_t        enroll_time;    /* tick when enrolled */
} user_entry_t;

/* ---------------------------------------------------------------------------
 * Context
 * --------------------------------------------------------------------------- */
typedef struct {
    lv_obj_t       *parent;

    /* User list */
    lv_obj_t       *list_cont;
    lv_obj_t       *row_objs[MAX_USERS];
    lv_obj_t       *row_name_lbls[MAX_USERS];
    lv_obj_t       *row_status_dots[MAX_USERS];
    lv_obj_t       *row_del_btns[MAX_USERS];

    /* Detail panel */
    lv_obj_t       *detail_name;
    lv_obj_t       *detail_status;
    lv_obj_t       *detail_conf;
    lv_obj_t       *detail_time;

    /* Controls */
    lv_obj_t       *name_ta;
    lv_obj_t       *enroll_btn;
    lv_obj_t       *enroll_bar;
    lv_obj_t       *status_label;

    /* Capacity */
    lv_obj_t       *capacity_arc;
    lv_obj_t       *capacity_label;

    /* Data */
    user_entry_t    users[MAX_USERS];
    int             user_count;
    int             selected;       /* -1 = none */

    /* Enrollment animation */
    lv_timer_t     *enroll_timer;
    int             enroll_slot;
    int             enroll_progress;
    uint32_t        tick;
} facedb_ctx_t;

static facedb_ctx_t s_ctx;

/* Simple PRNG for confidence values */
static uint32_t s_rng = 54321;
static uint32_t rng_next(void)
{
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 17;
    s_rng ^= s_rng << 5;
    return s_rng;
}

/* ---------------------------------------------------------------------------
 * Forward declarations
 * --------------------------------------------------------------------------- */
static void refresh_list(facedb_ctx_t *ctx);
static void update_capacity(facedb_ctx_t *ctx);
static void update_detail(facedb_ctx_t *ctx);

/* ---------------------------------------------------------------------------
 * Update capacity arc
 * --------------------------------------------------------------------------- */
static void update_capacity(facedb_ctx_t *ctx)
{
    int pct = (ctx->user_count * 100) / MAX_USERS;
    lv_arc_set_value(ctx->capacity_arc, pct);

    lv_color_t color = UI_COLOR_SUCCESS;
    if (pct > 75) color = UI_COLOR_ERROR;
    else if (pct > 50) color = UI_COLOR_WARNING;

    lv_obj_set_style_arc_color(ctx->capacity_arc, color, LV_PART_INDICATOR);

    char buf[24];
    snprintf(buf, sizeof(buf), "%d/%d", ctx->user_count, MAX_USERS);
    lv_label_set_text(ctx->capacity_label, buf);
}

/* ---------------------------------------------------------------------------
 * Update detail panel for selected user
 * --------------------------------------------------------------------------- */
static void update_detail(facedb_ctx_t *ctx)
{
    if (ctx->selected < 0 || ctx->selected >= MAX_USERS ||
        ctx->users[ctx->selected].status == STATUS_EMPTY) {
        lv_label_set_text(ctx->detail_name, "No selection");
        lv_label_set_text(ctx->detail_status, "---");
        lv_label_set_text(ctx->detail_conf, "---");
        lv_label_set_text(ctx->detail_time, "---");
        return;
    }

    user_entry_t *u = &ctx->users[ctx->selected];

    lv_label_set_text(ctx->detail_name, u->name);
    lv_obj_set_style_text_color(ctx->detail_name, UI_COLOR_PRIMARY, 0);

    const char *status_str = "Unknown";
    lv_color_t status_color = UI_COLOR_TEXT_DIM;
    if (u->status == STATUS_ENROLLED) {
        status_str = "Enrolled";
        status_color = UI_COLOR_SUCCESS;
    } else if (u->status == STATUS_ENROLLING) {
        status_str = "Enrolling...";
        status_color = UI_COLOR_WARNING;
    }
    lv_label_set_text(ctx->detail_status, status_str);
    lv_obj_set_style_text_color(ctx->detail_status, status_color, 0);

    char conf_buf[24];
    snprintf(conf_buf, sizeof(conf_buf), "Quality: %d%%", u->confidence);
    lv_label_set_text(ctx->detail_conf, conf_buf);

    lv_color_t conf_color = UI_COLOR_SUCCESS;
    if (u->confidence < 60) conf_color = UI_COLOR_ERROR;
    else if (u->confidence < 80) conf_color = UI_COLOR_WARNING;
    lv_obj_set_style_text_color(ctx->detail_conf, conf_color, 0);

    char time_buf[32];
    snprintf(time_buf, sizeof(time_buf), "Tick: %lu",
             (unsigned long)u->enroll_time);
    lv_label_set_text(ctx->detail_time, time_buf);
}

/* ---------------------------------------------------------------------------
 * Row click - select user
 * --------------------------------------------------------------------------- */
static void row_click_cb(lv_event_t *e)
{
    facedb_ctx_t *ctx = (facedb_ctx_t *)lv_event_get_user_data(e);
    lv_obj_t *target = lv_event_get_target(e);

    for (int i = 0; i < MAX_USERS; i++) {
        if (ctx->row_objs[i] == target || ctx->row_name_lbls[i] == target) {
            ctx->selected = i;
            /* Highlight selected row */
            lv_obj_set_style_bg_color(ctx->row_objs[i],
                                       lv_color_hex(0x1A3A5C), 0);
        } else if (ctx->row_objs[i]) {
            lv_obj_set_style_bg_color(ctx->row_objs[i],
                                       UI_COLOR_CARD_BG, 0);
        }
    }
    update_detail(ctx);
}

/* ---------------------------------------------------------------------------
 * Delete button callback
 * --------------------------------------------------------------------------- */
static void delete_cb(lv_event_t *e)
{
    facedb_ctx_t *ctx = (facedb_ctx_t *)lv_event_get_user_data(e);
    lv_obj_t *btn = lv_event_get_target(e);

    for (int i = 0; i < MAX_USERS; i++) {
        if (ctx->row_del_btns[i] == btn && ctx->users[i].status != STATUS_EMPTY) {
            char msg[48];
            snprintf(msg, sizeof(msg), "Deleted: %s", ctx->users[i].name);

            memset(&ctx->users[i], 0, sizeof(user_entry_t));
            ctx->user_count--;
            if (ctx->user_count < 0) ctx->user_count = 0;

            if (ctx->selected == i) ctx->selected = -1;

            lv_label_set_text(ctx->status_label, msg);
            lv_obj_set_style_text_color(ctx->status_label,
                                         UI_COLOR_WARNING, 0);

            refresh_list(ctx);
            update_capacity(ctx);
            update_detail(ctx);
            break;
        }
    }
}

/* ---------------------------------------------------------------------------
 * Enrollment animation timer
 * --------------------------------------------------------------------------- */
static void enroll_timer_cb(lv_timer_t *timer)
{
    facedb_ctx_t *ctx = (facedb_ctx_t *)lv_timer_get_user_data(timer);

    ctx->enroll_progress++;
    int pct = (ctx->enroll_progress * 100) / ENROLL_STEPS;
    lv_bar_set_value(ctx->enroll_bar, pct, LV_ANIM_ON);

    if (ctx->enroll_progress >= ENROLL_STEPS) {
        /* Enrollment complete */
        lv_timer_delete(ctx->enroll_timer);
        ctx->enroll_timer = NULL;

        int slot = ctx->enroll_slot;
        ctx->users[slot].status = STATUS_ENROLLED;
        ctx->users[slot].confidence = 70 + (int)(rng_next() % 30);  /* 70-99 */
        ctx->users[slot].enroll_time = ctx->tick++;

        char msg[48];
        snprintf(msg, sizeof(msg), LV_SYMBOL_OK " Enrolled: %s",
                 ctx->users[slot].name);
        lv_label_set_text(ctx->status_label, msg);
        lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_SUCCESS, 0);

        lv_bar_set_value(ctx->enroll_bar, 0, LV_ANIM_OFF);
        lv_obj_remove_state(ctx->enroll_btn, LV_STATE_DISABLED);

        ctx->selected = slot;
        refresh_list(ctx);
        update_detail(ctx);
    }
}

/* ---------------------------------------------------------------------------
 * Enroll button callback
 * --------------------------------------------------------------------------- */
static void enroll_cb(lv_event_t *e)
{
    facedb_ctx_t *ctx = (facedb_ctx_t *)lv_event_get_user_data(e);

    if (ctx->enroll_timer) return;  /* Already enrolling */

    const char *name = lv_textarea_get_text(ctx->name_ta);
    if (!name || strlen(name) == 0) {
        lv_label_set_text(ctx->status_label, "Enter a name first");
        lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_ERROR, 0);
        return;
    }

    if (ctx->user_count >= MAX_USERS) {
        lv_label_set_text(ctx->status_label, "Database full!");
        lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_ERROR, 0);
        return;
    }

    /* Find empty slot */
    int slot = -1;
    for (int i = 0; i < MAX_USERS; i++) {
        if (ctx->users[i].status == STATUS_EMPTY) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return;

    /* Start enrollment */
    strncpy(ctx->users[slot].name, name, NAME_MAX_LEN - 1);
    ctx->users[slot].name[NAME_MAX_LEN - 1] = '\0';
    ctx->users[slot].status = STATUS_ENROLLING;
    ctx->user_count++;

    ctx->enroll_slot = slot;
    ctx->enroll_progress = 0;

    lv_obj_add_state(ctx->enroll_btn, LV_STATE_DISABLED);

    char msg[48];
    snprintf(msg, sizeof(msg), "Enrolling %s...", ctx->users[slot].name);
    lv_label_set_text(ctx->status_label, msg);
    lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_WARNING, 0);

    /* Clear input */
    lv_textarea_set_text(ctx->name_ta, "");

    refresh_list(ctx);
    update_capacity(ctx);

    /* Seed PRNG with tick */
    s_rng = (uint32_t)lv_tick_get();

    /* Start enrollment animation */
    ctx->enroll_timer = lv_timer_create(enroll_timer_cb,
                                         ENROLL_INTERVAL_MS, ctx);
}

/* ---------------------------------------------------------------------------
 * Clear all callback
 * --------------------------------------------------------------------------- */
static void clear_all_cb(lv_event_t *e)
{
    facedb_ctx_t *ctx = (facedb_ctx_t *)lv_event_get_user_data(e);

    memset(ctx->users, 0, sizeof(ctx->users));
    ctx->user_count = 0;
    ctx->selected = -1;

    lv_label_set_text(ctx->status_label, "Database cleared");
    lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_TEXT_DIM, 0);

    refresh_list(ctx);
    update_capacity(ctx);
    update_detail(ctx);
}

/* ---------------------------------------------------------------------------
 * Refresh the user list display
 * --------------------------------------------------------------------------- */
static void refresh_list(facedb_ctx_t *ctx)
{
    for (int i = 0; i < MAX_USERS; i++) {
        if (ctx->users[i].status == STATUS_EMPTY) {
            /* Empty slot */
            char slot_str[24];
            snprintf(slot_str, sizeof(slot_str), "[Slot %d - empty]", i + 1);
            lv_label_set_text(ctx->row_name_lbls[i], slot_str);
            lv_obj_set_style_text_color(ctx->row_name_lbls[i],
                                         UI_COLOR_TEXT_DIM, 0);

            lv_obj_set_style_bg_color(ctx->row_status_dots[i],
                                       lv_color_hex(0x333333), 0);

            lv_obj_add_flag(ctx->row_del_btns[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            /* Occupied slot */
            char name_str[32];
            snprintf(name_str, sizeof(name_str), "%s", ctx->users[i].name);
            lv_label_set_text(ctx->row_name_lbls[i], name_str);
            lv_obj_set_style_text_color(ctx->row_name_lbls[i],
                                         UI_COLOR_TEXT, 0);

            lv_color_t dot_color = UI_COLOR_WARNING;
            if (ctx->users[i].status == STATUS_ENROLLED) {
                dot_color = UI_COLOR_SUCCESS;
            }
            lv_obj_set_style_bg_color(ctx->row_status_dots[i],
                                       dot_color, 0);

            lv_obj_remove_flag(ctx->row_del_btns[i], LV_OBJ_FLAG_HIDDEN);
        }

        /* Highlight selected row */
        if (i == ctx->selected && ctx->users[i].status != STATUS_EMPTY) {
            lv_obj_set_style_bg_color(ctx->row_objs[i],
                                       lv_color_hex(0x1A3A5C), 0);
        } else {
            lv_obj_set_style_bg_color(ctx->row_objs[i],
                                       UI_COLOR_CARD_BG, 0);
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
    s_ctx.selected = -1;

    /* Pre-populate 2 demo entries */
    strncpy(s_ctx.users[0].name, "Alice", NAME_MAX_LEN - 1);
    s_ctx.users[0].status = STATUS_ENROLLED;
    s_ctx.users[0].confidence = 95;
    s_ctx.users[0].enroll_time = 1;

    strncpy(s_ctx.users[1].name, "Bob", NAME_MAX_LEN - 1);
    s_ctx.users[1].status = STATUS_ENROLLED;
    s_ctx.users[1].confidence = 82;
    s_ctx.users[1].enroll_time = 2;

    s_ctx.user_count = 2;
    s_ctx.tick = 3;

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_EYE_OPEN " Face Database");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* ฐานข้อมูลใบหน้า */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "ฐานข้อมูลใบหน้า");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 28);

    /* UI Concept banner */
    lv_obj_t *concept = lv_label_create(parent);
    lv_label_set_text(concept, LV_SYMBOL_WARNING " UI Concept - camera + face recognition not yet available");
    lv_obj_set_style_text_color(concept, lv_color_hex(0xFF9800), 0);
    lv_obj_align(concept, LV_ALIGN_TOP_RIGHT, -10, 8);

    /* Status label */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "2 users enrolled");
    lv_obj_set_style_text_color(s_ctx.status_label, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(s_ctx.status_label, &lv_font_montserrat_14, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 16, 36);

    /* === User list (left panel) === */
    lv_obj_t *list_card = example_card_create(parent, 280, 240, UI_COLOR_CARD_BG);
    lv_obj_align(list_card, LV_ALIGN_TOP_LEFT, 8, 56);

    lv_obj_t *list_title = example_label_create(list_card,
        "Enrolled Users", &lv_font_montserrat_16, UI_COLOR_PRIMARY);
    lv_obj_align(list_title, LV_ALIGN_TOP_LEFT, 0, 0);

    s_ctx.list_cont = lv_obj_create(list_card);
    lv_obj_set_size(s_ctx.list_cont, 256, 200);
    lv_obj_align(s_ctx.list_cont, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_opa(s_ctx.list_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_ctx.list_cont, 0, 0);
    lv_obj_set_style_pad_all(s_ctx.list_cont, 0, 0);
    lv_obj_set_style_pad_row(s_ctx.list_cont, 2, 0);
    lv_obj_set_flex_flow(s_ctx.list_cont, LV_FLEX_FLOW_COLUMN);

    /* Create rows */
    for (int i = 0; i < MAX_USERS; i++) {
        s_ctx.row_objs[i] = lv_obj_create(s_ctx.list_cont);
        lv_obj_set_size(s_ctx.row_objs[i], 252, 24);
        lv_obj_set_style_bg_color(s_ctx.row_objs[i], UI_COLOR_CARD_BG, 0);
        lv_obj_set_style_bg_opa(s_ctx.row_objs[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(s_ctx.row_objs[i], 4, 0);
        lv_obj_set_style_border_width(s_ctx.row_objs[i], 0, 0);
        lv_obj_set_style_pad_all(s_ctx.row_objs[i], 2, 0);
        lv_obj_clear_flag(s_ctx.row_objs[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(s_ctx.row_objs[i], row_click_cb,
                            LV_EVENT_CLICKED, &s_ctx);

        /* Status dot */
        s_ctx.row_status_dots[i] = lv_obj_create(s_ctx.row_objs[i]);
        lv_obj_set_size(s_ctx.row_status_dots[i], 10, 10);
        lv_obj_align(s_ctx.row_status_dots[i], LV_ALIGN_LEFT_MID, 2, 0);
        lv_obj_set_style_radius(s_ctx.row_status_dots[i],
                                 LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(s_ctx.row_status_dots[i],
                                   lv_color_hex(0x333333), 0);
        lv_obj_set_style_bg_opa(s_ctx.row_status_dots[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s_ctx.row_status_dots[i], 0, 0);
        lv_obj_clear_flag(s_ctx.row_status_dots[i], LV_OBJ_FLAG_SCROLLABLE);

        /* Name label */
        s_ctx.row_name_lbls[i] = lv_label_create(s_ctx.row_objs[i]);
        lv_obj_set_style_text_font(s_ctx.row_name_lbls[i],
                                    &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(s_ctx.row_name_lbls[i],
                                     UI_COLOR_TEXT_DIM, 0);
        lv_obj_align(s_ctx.row_name_lbls[i], LV_ALIGN_LEFT_MID, 18, 0);
        lv_obj_add_event_cb(s_ctx.row_name_lbls[i], row_click_cb,
                            LV_EVENT_CLICKED, &s_ctx);

        /* Delete button (small X) */
        s_ctx.row_del_btns[i] = lv_btn_create(s_ctx.row_objs[i]);
        lv_obj_set_size(s_ctx.row_del_btns[i], 22, 18);
        lv_obj_align(s_ctx.row_del_btns[i], LV_ALIGN_RIGHT_MID, -2, 0);
        lv_obj_set_style_bg_color(s_ctx.row_del_btns[i],
                                   UI_COLOR_ERROR, 0);
        lv_obj_set_style_radius(s_ctx.row_del_btns[i], 4, 0);
        lv_obj_set_style_pad_all(s_ctx.row_del_btns[i], 0, 0);
        lv_obj_add_event_cb(s_ctx.row_del_btns[i], delete_cb,
                            LV_EVENT_CLICKED, &s_ctx);
        lv_obj_add_flag(s_ctx.row_del_btns[i], LV_OBJ_FLAG_HIDDEN);

        lv_obj_t *x_lbl = lv_label_create(s_ctx.row_del_btns[i]);
        lv_label_set_text(x_lbl, LV_SYMBOL_CLOSE);
        lv_obj_set_style_text_font(x_lbl, &lv_font_montserrat_14, 0);
        lv_obj_center(x_lbl);
    }

    /* === Detail panel (right) === */
    lv_obj_t *detail_card = example_card_create(parent, 180, 150, UI_COLOR_CARD_BG);
    lv_obj_align(detail_card, LV_ALIGN_TOP_RIGHT, -8, 56);

    lv_obj_t *det_title = example_label_create(detail_card,
        "User Details", &lv_font_montserrat_14, UI_COLOR_PRIMARY);
    lv_obj_align(det_title, LV_ALIGN_TOP_LEFT, 0, 0);

    s_ctx.detail_name = example_label_create(detail_card,
        "No selection", &lv_font_montserrat_20, UI_COLOR_TEXT);
    lv_obj_align(s_ctx.detail_name, LV_ALIGN_TOP_LEFT, 0, 22);

    s_ctx.detail_status = example_label_create(detail_card,
        "---", &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(s_ctx.detail_status, LV_ALIGN_TOP_LEFT, 0, 50);

    s_ctx.detail_conf = example_label_create(detail_card,
        "---", &lv_font_montserrat_16, UI_COLOR_TEXT);
    lv_obj_align(s_ctx.detail_conf, LV_ALIGN_TOP_LEFT, 0, 72);

    s_ctx.detail_time = example_label_create(detail_card,
        "---", &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(s_ctx.detail_time, LV_ALIGN_TOP_LEFT, 0, 96);

    /* === Capacity arc (right, below detail) === */
    lv_obj_t *cap_card = example_card_create(parent, 180, 120, UI_COLOR_CARD_BG);
    lv_obj_align(cap_card, LV_ALIGN_TOP_RIGHT, -8, 216);

    lv_obj_t *cap_title = example_label_create(cap_card,
        "Capacity", &lv_font_montserrat_14, UI_COLOR_PRIMARY);
    lv_obj_align(cap_title, LV_ALIGN_TOP_LEFT, 0, 0);

    s_ctx.capacity_arc = lv_arc_create(cap_card);
    lv_obj_set_size(s_ctx.capacity_arc, 80, 80);
    lv_obj_align(s_ctx.capacity_arc, LV_ALIGN_CENTER, 0, 8);
    lv_arc_set_range(s_ctx.capacity_arc, 0, 100);
    lv_arc_set_bg_angles(s_ctx.capacity_arc, 135, 45);
    lv_obj_remove_style(s_ctx.capacity_arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(s_ctx.capacity_arc, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_ctx.capacity_arc, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_ctx.capacity_arc,
                                lv_color_hex(0x1A1A2E), LV_PART_MAIN);
    lv_obj_clear_flag(s_ctx.capacity_arc, LV_OBJ_FLAG_CLICKABLE);

    s_ctx.capacity_label = lv_label_create(s_ctx.capacity_arc);
    lv_obj_set_style_text_font(s_ctx.capacity_label,
                                &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_ctx.capacity_label, UI_COLOR_TEXT, 0);
    lv_obj_center(s_ctx.capacity_label);

    /* === Enrollment controls (bottom) === */
    lv_obj_t *ctrl_card = example_card_create(parent, 460, 80, UI_COLOR_CARD_BG);
    lv_obj_align(ctrl_card, LV_ALIGN_BOTTOM_MID, 0, -44);

    lv_obj_t *name_lbl = example_label_create(ctrl_card,
        "Name:", &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_align(name_lbl, LV_ALIGN_TOP_LEFT, 0, 0);

    s_ctx.name_ta = lv_textarea_create(ctrl_card);
    lv_obj_set_size(s_ctx.name_ta, 160, 32);
    lv_obj_align(s_ctx.name_ta, LV_ALIGN_TOP_LEFT, 48, -2);
    lv_textarea_set_one_line(s_ctx.name_ta, true);
    lv_textarea_set_placeholder_text(s_ctx.name_ta, "Enter name...");
    lv_textarea_set_max_length(s_ctx.name_ta, NAME_MAX_LEN - 1);

    s_ctx.enroll_btn = lv_btn_create(ctrl_card);
    lv_obj_set_size(s_ctx.enroll_btn, 100, 32);
    lv_obj_align(s_ctx.enroll_btn, LV_ALIGN_TOP_LEFT, 220, -2);
    lv_obj_set_style_bg_color(s_ctx.enroll_btn, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_radius(s_ctx.enroll_btn, 6, 0);
    lv_obj_add_event_cb(s_ctx.enroll_btn, enroll_cb,
                        LV_EVENT_CLICKED, &s_ctx);

    lv_obj_t *enroll_lbl = lv_label_create(s_ctx.enroll_btn);
    lv_label_set_text(enroll_lbl, LV_SYMBOL_PLUS " Enroll");
    lv_obj_center(enroll_lbl);

    /* Clear all button */
    lv_obj_t *clear_btn = lv_btn_create(ctrl_card);
    lv_obj_set_size(clear_btn, 100, 32);
    lv_obj_align(clear_btn, LV_ALIGN_TOP_LEFT, 330, -2);
    lv_obj_set_style_bg_color(clear_btn, UI_COLOR_ERROR, 0);
    lv_obj_set_style_radius(clear_btn, 6, 0);
    lv_obj_add_event_cb(clear_btn, clear_all_cb,
                        LV_EVENT_CLICKED, &s_ctx);

    lv_obj_t *clear_lbl = lv_label_create(clear_btn);
    lv_label_set_text(clear_lbl, LV_SYMBOL_TRASH " Clear");
    lv_obj_center(clear_lbl);

    /* Enrollment progress bar */
    s_ctx.enroll_bar = lv_bar_create(ctrl_card);
    lv_obj_set_size(s_ctx.enroll_bar, 420, 12);
    lv_obj_align(s_ctx.enroll_bar, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_bar_set_range(s_ctx.enroll_bar, 0, 100);
    lv_bar_set_value(s_ctx.enroll_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_ctx.enroll_bar, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(s_ctx.enroll_bar, UI_COLOR_PRIMARY,
                               LV_PART_INDICATOR);

    /* Initialize display */
    refresh_list(&s_ctx);
    update_capacity(&s_ctx);
    update_detail(&s_ctx);
}
