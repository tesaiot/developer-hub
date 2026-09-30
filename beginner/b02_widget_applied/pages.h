/**
 * @file    pages.h
 * @brief   Forward declarations for all Widget Applied sub-pages
 *
 * Each page_xxx_create() builds its demo inside the given parent.
 * The parent is a full-screen container provided by the grid menu.
 */

#ifndef PAGES_H
#define PAGES_H

#include "pse84_common.h"

void page_led_brightness_create(lv_obj_t *parent);
void page_switch_led_create(lv_obj_t *parent);
void page_stopwatch_create(lv_obj_t *parent);
void page_reaction_game_create(lv_obj_t *parent);
void page_bar_chart_create(lv_obj_t *parent);
void page_imu_display_create(lv_obj_t *parent);
void page_temperature_create(lv_obj_t *parent);
void page_humidity_create(lv_obj_t *parent);
void page_compass_create(lv_obj_t *parent);
void page_system_info_create(lv_obj_t *parent);

#endif /* PAGES_H */
