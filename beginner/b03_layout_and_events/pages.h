/**
 * @file    pages.h
 * @brief   Forward declarations for all layout & event demo sub-pages
 *
 * Each page_xxx_create() builds its demo inside the given parent.
 * The parent is a full-screen container provided by the grid menu.
 */

#ifndef PAGES_H
#define PAGES_H

#include "pse84_common.h"

void page_flex_row_create(lv_obj_t *parent);
void page_flex_col_create(lv_obj_t *parent);
void page_grid_create(lv_obj_t *parent);
void page_card_create(lv_obj_t *parent);
void page_screen_bg_create(lv_obj_t *parent);
void page_custom_style_create(lv_obj_t *parent);
void page_click_event_create(lv_obj_t *parent);
void page_value_changed_create(lv_obj_t *parent);
void page_long_press_create(lv_obj_t *parent);
void page_multi_event_create(lv_obj_t *parent);
void page_timer_create(lv_obj_t *parent);

#endif /* PAGES_H */
